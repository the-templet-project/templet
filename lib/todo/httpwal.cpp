// cpp-httplib needs Windows 10 API level; must be set before any system header
#ifdef _WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

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
#include <string>
#include <vector>
#include <memory>

// https://github.com/yhirose/cpp-httplib
#include "../opt/httplib.h"

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
friend class stub;
friend class proxy;
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

// splits "host:port" (the port is optional) into host and port
static void split_addr(const std::string& addr, std::string& host, int& port) {
    const int default_port = 51717;
    size_t pos = addr.rfind(':');
    if (pos == std::string::npos) { host = addr; port = default_port; return; }
    host = addr.substr(0, pos);
    try { port = std::stoi(addr.substr(pos + 1)); }
    catch (...) { std::cerr << "bad address '" << addr << "'"; exit(EXIT_FAILURE); }
}

// HTTP server that exposes a wal:
//   POST /write?tag=N  (body = blob)  -> 200, body = index
//   GET  /read?index=N                -> 200, body = blob, header X-Tag = tag
//                                     -> 404 if there is no such index
class stub{
public:
    stub(const cfg& c, wal& w):_wal(w){
        std::string host; int port;
        split_addr(c._addr, host, port);

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
            std::cerr << "cannot bind the server to '" << c._addr << "'";
            exit(EXIT_FAILURE);
        }
        _thread = std::thread([this] { _svr.listen_after_bind(); });
        _svr.wait_until_ready();
    }
    ~stub(){
        _svr.stop();
        if (_thread.joinable()) _thread.join();
    }
private:
    wal& _wal;
    httplib::Server _svr;
    std::thread _thread;
};

// wal that forwards write() and read() to a remote stub over HTTP
class proxy : public wal {
public:
    proxy(const cfg& c):_addr(c._addr){
        std::string host; int port;
        split_addr(c._addr, host, port);
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
