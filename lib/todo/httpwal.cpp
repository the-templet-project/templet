#include <functional>
#include <istream>
#include <ostream>
#include <mutex>
#include <cstdio>
#include <fstream>
#include <cassert>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>

// #include "path/to/httplib.h"
// https://github.com/yhirose/cpp-httplib

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
    void print(std::ostream& out) {
        for (int i = 0; i < _log.size(); i++) {
            out << "index:" << i << " tag:" << _log[i].first << std::endl
                << "entry:" << _log[i].second << std::endl;
        }
    }
protected:
    std::vector<std::pair<unsigned, std::string>> _log;
    std::mutex _mut;
};

class cfg {
friend class env;
public:
    cfg(const char filename[]):cfg(){}
    cfg():_mode(type::mem),_numtasks(SYSDEF),
        _addr("localhost:51717"){}
public:
    enum type {mem, one, srv, cli};
    static const unsigned SYSDEF = 0;
public:
    void mode(type m){_mode=m;}
    void numtasks(unsigned nt){_numtasks=nt;}
    void addr(const std::string& a){_addr=a;}
private:
    type _mode;
    unsigned _numtasks;
    std::string _addr;
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
        if(_cfg._mode==cfg::type::mem){
            memwal wal;
            _run_cli_tasks(wal,task);
        }
        else if(_cfg._mode==cfg::type::one){
            memwal wal;
            stub  srv(_cfg,wal);
            proxy cli(_cfg);
            _run_cli_tasks(cli,task);
        }
        else if(_cfg._mode==cfg::type::srv){
            memwal wal;
            stub  srv(_cfg,wal);
            _run_cli_tasks(wal,task);
        }
        else if(_cfg._mode==cfg::type::cli){
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
        unsigned size=_cfg._numtasks;
        if(size==cfg::SYSDEF)size=std::thread::hardware_concurrency();
        
        _beg=std::chrono::high_resolution_clock::now();
        
        if(size==1){ 
            task(0,srv);
        }
        else{
            std::vector<std::thread> threads(size);
            for (auto& t : threads) t = std::thread([&]{
                task(_taskID++,srv);
            }); 
            for (auto& t : threads) t.join();
        }
        _end=std::chrono::high_resolution_clock::now();
    }
private:
    cfg& _cfg;
private:
    unsigned _size;
    std::atomic_int _taskID = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
};

int main()
{
    bool master = true;
    
    cfg cfg; 
    env env(cfg);

    // setting cfg parameters
    cfg.mode(cfg::type::mem);
    // cfg.mode(cfg::type::one);
    // cfg.mode(cfg::type::cli);
    // cfg.mode(cfg::type::srv);
    
    //cfg.numtasks(cfg::SYSDEF);
    cfg.numtasks(10);

    env([&](unsigned pid,wal& wal) {
        unsigned index;
        wal.write(index,pid,std::string("blob"));
        env::delay(1.0); 
        
        if (pid == 0 && master){ // in master 'process'
            unsigned index=0; unsigned tag; std::string blob;
            while(wal.read(index, tag, blob)){ 
                std::cout << "index = " << index << " tag = " << tag << std::endl;
                index++;
            }
        }
    });
}
