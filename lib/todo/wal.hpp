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

// #include "path/to/httplib.h"
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

    class stub{
    public:
        stub(wal*,std::string& addr,unsigned port){}
    public:
        void run(){}
        void stop(){}
    };

    class proxy : public wal {
    public:
        proxy(std::string& addr,unsigned port){}
    public:
        void write(unsigned& index, unsigned tag, const std::string& blob) override {}
        bool read(unsigned index, unsigned& tag, std::string& blob) override {return false;}
    };

    class cfg{
    friend class env;
    public:
        void srv(const std::string& a,unsigned p){_srv=true;_addr=a;_port=p;}
        void srv(bool s=true){_srv=s;}
        void cli(const std::string& a,unsigned p){_cli=true;_addr=a;_port=p;}
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
        std::string _addr="localhost:51717";
        unsigned _port=51717;
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
                _srv_stub = new stub(_srv_wal,_cfg._addr,_cfg._port);
                _srv_stub->run();
            }
            else{
                _srv_wal  = nullptr;
                _srv_stub = nullptr;
            }
            
            _cli_wal = _cfg._cli ? new proxy(_cfg._addr,_cfg._port) : nullptr;
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
