#pragma once

/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#include <functional>
#include <istream>
#include <ostream>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>

#include "../opt/httplib.h"
// https://github.com/yhirose/cpp-httplib

namespace templet {

    class wal {
    public:
        virtual ~wal() {}
        virtual void write(unsigned& index, unsigned tag, const std::string& blob) = 0;
        virtual bool read(unsigned index, unsigned& tag, std::string& blob) = 0;
    };

    class memwal :public wal {
    public:
        virtual ~memwal() override {}
        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            std::unique_lock<std::mutex> lock(_mut);
            _log.push_back(std::pair<unsigned, std::string>(tag, blob));
            index = (unsigned)(_log.size() - 1);
        }
        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            std::unique_lock<std::mutex> lock(_mut);
            if (index < _log.size()) { tag = _log[index].first; blob = _log[index].second; return true; }
            return false;
        }
    private:
        std::vector<std::pair<unsigned, std::string>> _log;
        std::mutex _mut;
    };

    class filewal :public wal {
    public:
        filewal(const std::string&file):filewal(file,true,true){}
        filewal(const std::string&file,bool resume,bool lazy):
            _file(file),_resume(resume),_lazy(lazy),_hfile(NULL){

            if(!_resume){ // new calcs         
                _hfile = fopen(_file.c_str(), "rb");
                
                if(_hfile) { std::cerr << "the file '" << _file
                    << "' already exists; data loss is possible"; fclose(_hfile);
                    exit(EXIT_FAILURE);
                }
            }
            else{ // resume calcs
                _hfile = fopen(_file.c_str(), "rb");

                if(!_hfile){ std::cerr << "cannot open the file '" << _file
                    << "' for reading"; 
                    exit(EXIT_FAILURE);
                }
                
                unsigned ubuf[3]; //index,tag,blob size
                std::string blob;
                size_t ret_code;
    
                for(;;){
                    ret_code = fread(ubuf, 1, sizeof(ubuf), _hfile);
                    if(ret_code==0) break;
                    
                    if(ret_code!=sizeof(ubuf)){ 
                        std::cerr << "record corrupted in file '" << _file <<"'";exit(EXIT_FAILURE);}
                    if(ubuf[2]>blob.max_size()){
                        std::cerr << "bad blob size in file '" << _file <<"'";exit(EXIT_FAILURE);}
                    if(ubuf[0]!=_log.size()){
                        std::cerr << "bad index in file '" << _file <<"'";exit(EXIT_FAILURE);}
    
                    blob.resize(ubuf[2]);//size
    
                    ret_code = fread((void*)blob.c_str(), sizeof(char), ubuf[2], _hfile);//blob
                    if(ret_code!=ubuf[2]){
                        std::cerr << "record corrupted in file '" << _file <<"'";exit(EXIT_FAILURE);}
    
                    _log.push_back(std::pair<unsigned, std::string>(ubuf[1], blob));//tag,blob
                }
                fclose(_hfile);
            }
            _hfile = fopen(_file.c_str(), "ab");

            if(!_hfile){ std::cerr << "cannot open the file '" << _file
                << "' for appending"; 
                exit(EXIT_FAILURE);
            }
        }
        virtual ~filewal() override {fclose(_hfile);}

        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            std::unique_lock<std::mutex> lock(_mut);
            _log.push_back(std::pair<unsigned, std::string>(tag, blob));
            index = (unsigned)(_log.size() - 1);

            size_t ret_code;
            unsigned ubuf[3];//index,tag,blob size
            
            ubuf[0] = _log.size()-1; //index
            ubuf[1] = tag;   //tag
            ubuf[2] = blob.size(); //blob size

            ret_code = fwrite(ubuf, sizeof ubuf[0], 3, _hfile);
            if(ret_code !=3){ std::cerr << "error write to file '" << _file <<"'";exit(EXIT_FAILURE);}

            ret_code = fwrite(blob.c_str(), sizeof(char), ubuf[2], _hfile);
            if(ret_code != ubuf[2]){ std::cerr << "error write to file '" << _file <<"'";exit(EXIT_FAILURE);}
            
            if(!_lazy) fflush(_hfile);
        }
        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            std::unique_lock<std::mutex> lock(_mut);
            if (index < _log.size()) { tag = _log[index].first; blob = _log[index].second; return true; }
            return false;
        }
    private:
        std::vector<std::pair<unsigned, std::string>> _log;
        std::mutex _mut;
    private:
        std::string _file;
        FILE* _hfile;
        bool _resume;
        bool _lazy;
    };

    //   HTTP server that exposes a wal:
    //   POST /write?tag=N  (body = blob)  -> 200, body = index
    //   GET  /read?index=N                -> 200, body = blob, header X-Tag = tag
    //                                     -> 404 if there is no such index
    class stub{
    public:
        stub(wal*w,const std::string& host,int port):_wal(*w),_is_running(false){
            
           _svr.Post("/write", [this](const httplib::Request& req, httplib::Response& res) {
                unsigned tag, index;
                try { tag = (unsigned)std::stoul(req.get_param_value("tag")); }
                catch (...) { res.status = 400; return; }
                _wal.write(index, tag, req.body);
                res.set_content(std::to_string(index), "text/plain");
            });
    
            _svr.Get("/read", [this](const httplib::Request& req, httplib::Response& res) {
                unsigned index, tag; std::string blob;
                try { index = (unsigned)std::stoul(req.get_param_value("index")); }
                catch (...) { res.status = 400; return; }
                if (!_wal.read(index, tag, blob)) { res.status = 404; return; }
                res.set_header("X-Tag", std::to_string(tag));
                res.set_content(blob, "application/octet-stream");
            });

            if (!_svr.bind_to_port(host, port)) {
                std::cerr << "cannot bind the server to host '" << host << "' port# " << port;
                exit(EXIT_FAILURE);
            }
        }
        ~stub(){ stop(); }
    public:
        void run(){
            _is_running = true;
            _thread = std::thread([this] { 
                if(!_svr.listen_after_bind()){
                    std::cerr << "unable to start listening after bind";
                    exit(EXIT_FAILURE);
                }
            });
            _svr.wait_until_ready();
        }
        void stop(){
            if(_is_running){
                _svr.stop();
                if (_thread.joinable()) _thread.join();
                _is_running = false;
            }
        }
    private:
        wal& _wal;
        httplib::Server _svr;
        std::thread _thread;
        bool _is_running;
    };

    // wal that forwards write() and read() to a remote stub over HTTP

    class proxy : public wal {
    public:
        proxy(const std::string& host,int port){
            _addr = host + ":" + std::to_string(port);
            _cli.reset(new httplib::Client(host, port));
            _cli->set_connection_timeout(5);
        }
    public:
        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            std::unique_lock<std::mutex> lock(_mut); // one client is shared by all the tasks
            auto res = _cli->Post("/write?tag=" + std::to_string(tag), blob, "application/octet-stream");
            if (!res || res->status != 200) fail("write", res);
            index = (unsigned)std::stoul(res->body);            
        }
        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            std::unique_lock<std::mutex> lock(_mut);
            auto res = _cli->Get("/read?index=" + std::to_string(index));
            if (res && res->status == 404) return false;
            if (!res || res->status != 200) fail("read", res);
            tag = (unsigned)std::stoul(res->get_header_value("X-Tag"));
            blob = res->body;
            return true;           
        }
    private:
        void fail(const char* op, const httplib::Result& res) {
            std::cerr << "error in " << op << " on '" << _addr << "': ";
            if (res) std::cerr << "HTTP status " << res->status;
            else std::cerr << httplib::to_string(res.error());
            exit(EXIT_FAILURE);
        }
    private:
        std::string _addr;
        std::unique_ptr<httplib::Client> _cli;
        std::mutex _mut;
    };

    class cfg{
    friend class env;
    public:
        void srv(const std::string& h,int p){_srv=true;_host=h;_port=p;}
        void srv(bool s=true){_srv=s;}
        void cli(const std::string& h,int p=51717){_cli=true;_host=h;_port=p;}
        void cli(bool c=true){_cli=c;}
        void pool(unsigned size){_pool=size;}
        void file(const std::string& f){_file=f;_mem=false;}
        void file(bool f){_mem = !f;}
        void resume(bool r=true){_resume=r;}
        void lazy(bool l=true){_lazy=l;}
    private:
        bool _mem=true;
        bool _srv=false;
        bool _cli=false;
        std::string _host="127.0.0.1";
        int _port=51717;
        unsigned _pool=0;//default pool size
        std::string _file="wal.dat";
        bool _resume=false;
        bool _lazy=true;
    };    

    class env{
    public:
        env(cfg&c):_cfg(c){}
        ~env(){ close(); }
    public:
        inline void operator()(std::function<void(unsigned taskID,wal&)>task){env::run(task);}
        void run(std::function<void(unsigned taskID,wal&)> task){
            if(_cfg._srv){
                _srv_wal  = _cfg._mem ? static_cast<wal*>(new memwal) : 
                                        static_cast<wal*>(new filewal(_cfg._file,_cfg._resume,_cfg._lazy));
                _srv_stub = new stub(_srv_wal,_cfg._host,_cfg._port);
                _srv_stub->run();
            }
            else{
                _srv_wal  = nullptr;
                _srv_stub = nullptr;
            }
            
            _cli_wal = _cfg._cli ? new proxy(_cfg._host,_cfg._port) : nullptr;
            _cli_wal = (!_cfg._cli && !_cfg._srv)?
                (_cfg._mem ? static_cast<wal*>(new memwal) : 
                             static_cast<wal*>(new filewal(_cfg._file,_cfg._resume,_cfg._lazy))) : 
                _cli_wal;
            
            if(_cfg._srv && !_cfg._cli) run_clients(_srv_wal,task);
            else run_clients(_cli_wal,task);
            
            if(_cfg._srv) _srv_stub->stop();
            close();
        }
    private:
        inline void run_clients(wal*log,std::function<void(unsigned taskID,wal&)> task){
            _beg=std::chrono::high_resolution_clock::now();       
            if(_cfg._pool==1)
                 task(0,*log);
            else{
                std::vector<std::thread> threads(
                        _cfg._pool==0 ? std::thread::hardware_concurrency() : _cfg._pool);
                std::atomic_int _taskID = 0;
                for (auto& t : threads) t = std::thread([&]{task(_taskID++,*log);}); 
                for (auto& t : threads) t.join();
            }
            _end=std::chrono::high_resolution_clock::now();
        }
        void close(){
            if(_cli_wal) {delete _cli_wal; _cli_wal = nullptr;} 
            if(_srv_wal) {delete _srv_wal; _srv_wal = nullptr;}
            if(_srv_stub){delete _srv_stub;_srv_stub= nullptr;}
        }    
    public:
        static void delay(double seconds){std::this_thread::sleep_for(std::chrono::duration<double>(seconds));}
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;    
    private:
        cfg& _cfg;
        wal* _srv_wal=nullptr;
        stub*_srv_stub=nullptr;
        wal* _cli_wal=nullptr;
    };

}
