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
        virtual void write(unsigned& index, unsigned tag, const std::string& blob) = 0;
        virtual bool read(unsigned index, unsigned& tag, std::string& blob) = 0;
    };

    class memwal :public wal {
    public:
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
        filewal(const std::string&file,bool rewrite=false,bool lazy=true):
            _file(file),_rewrite(rewrite),_lazy(lazy),_hfile(NULL){
            
            if(_rewrite) _hfile = fopen(_file.c_str(), "wb");
            else _hfile = fopen(_file.c_str(), "r+b");
            
            if(!_hfile){ std::cerr << "cannot open the file '" << _file
                << "' in '" << (_rewrite?"wb":"r+b") << "' mode"; 
                exit(EXIT_FAILURE);
            }

            if(_rewrite) return;

            unsigned ubuf[3];//index,tag,blob size
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
        }
        ~filewal(){fclose(_hfile);}

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
        bool _rewrite;
        bool _lazy;
    };
    
    class cfg {
    friend class env;
    public:
        cfg(const char filename[]):cfg(){}
        cfg():_mode(LOCAL),_wal(RAM),_size(SYSDEF),
            _addr("localhost:51717"),_port(51717),
            _file("wal.dat"),_rewrite(false),_lazy(true){}
    public:
        enum deploy {LOCAL, LOOP, SRV, CLI};
        enum place {RAM, DISK};
        static const unsigned SYSDEF = 0;
    public:
        void mode(deploy m){_mode=m;}
        void wal(place w){_wal=w;}
        void size(unsigned s){_size=s;}
        void addr(const std::string& a){_addr=a;}
        void port(unsigned p){_port=p;}
        void file(const std::string& f){_file=f;}
        void rewrite(bool rw){_rewrite=rw;}
        void lazy(bool l){_lazy=l;}
    private:
        deploy _mode;
        place _wal;
        unsigned _size;
        std::string _addr;
        unsigned _port;
        std::string _file;
        bool _rewrite;
        bool _lazy;
    };
    
    class stub{
    public:
        stub(const cfg&, wal&){}
    };
    
    class proxy : public wal {
    public:
        proxy(const cfg&){}
    public:
        void write(unsigned& index, unsigned tag, const std::string& blob) override {}
        bool read(unsigned index, unsigned& tag, std::string& blob) override {return false;}
    };
    
    class env {
    public:
        env(cfg&c):_cfg(c){}
    public:
        inline void operator()(std::function<void(unsigned taskID,wal&wal)>task){env::run(task);}
        static void delay(double seconds){std::this_thread::sleep_for(std::chrono::duration<double>(seconds));}
        void run(std::function<void(unsigned taskID,wal&)> task){
            if(_cfg._mode==cfg::LOCAL){
                if(_cfg._wal==cfg::DISK){
                    filewal wal(_cfg._file,_cfg._rewrite,_cfg._lazy);
                    _run_cli_tasks(wal,task);
                }
                else if(_cfg._wal==cfg::RAM){
                    memwal wal;
                    _run_cli_tasks(wal,task);                    
                }
            }
            else if(_cfg._mode==cfg::LOOP){
                if(_cfg._wal==cfg::DISK){
                    filewal wal(_cfg._file,_cfg._rewrite,_cfg._lazy);
                    stub  srv(_cfg,wal);
                    proxy cli(_cfg);
                    _run_cli_tasks(cli,task);
                }
                else if(_cfg._wal==cfg::RAM){
                    memwal wal;
                    stub  srv(_cfg,wal);
                    proxy cli(_cfg);
                    _run_cli_tasks(cli,task);
                }
            }
            else if(_cfg._mode==cfg::SRV){
                if(_cfg._wal==cfg::DISK){
                    filewal wal(_cfg._file,_cfg._rewrite,_cfg._lazy);
                    stub  srv(_cfg,wal);
                    _run_cli_tasks(wal,task);
                }
                else if(_cfg._wal==cfg::RAM){
                    memwal wal;
                    stub  srv(_cfg,wal);
                    _run_cli_tasks(wal,task);
                }
            }
            else if(_cfg._mode==cfg::CLI){
                proxy cli(_cfg);
                _run_cli_tasks(cli,task);
            }
        }
        double duration(){ 
            std::chrono::duration<double> dur = _end - _beg;
            return dur.count();
        }
    private:
        inline void _run_cli_tasks(wal& srv,
            std::function<void(unsigned taskID,wal&)> task)
        {
            unsigned size=_cfg._size;
            if(size==cfg::SYSDEF)size=std::thread::hardware_concurrency();
            
            _beg=std::chrono::high_resolution_clock::now();
            
            if(size==1){ 
                task(0,srv);
            }
            else{
                std::vector<std::thread> threads(size);
                std::atomic_int _taskID = 0;
                
                for (auto& t : threads) t = std::thread([&]{
                    task(_taskID++,srv);
                }); 
                for (auto& t : threads) t.join();
            }
            _end=std::chrono::high_resolution_clock::now();
        }
    private:
        cfg& _cfg;
        std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
    };
}
