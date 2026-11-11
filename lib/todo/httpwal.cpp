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
//   POST /hello                       -> 200, body = client id
//   POST /bye?id=N                    -> 200
class stub{
public:
    stub(const cfg& c, wal& w):_wal(w){
        std::string host; int port;
        split_addr(c._addr, host, port);
        touch();

        _svr.Post("/hello", [this](const httplib::Request& req, httplib::Response& res) {
            unsigned id;
            {   std::unique_lock<std::mutex> lock(_imut);
                id = ++_clients;
                _active++;
                touch();
            }
            std::cout << "client " << id << " hello" << std::endl;
            res.set_content(std::to_string(id), "text/plain");
        });

        _svr.Post("/bye", [this](const httplib::Request& req, httplib::Response& res) {
            unsigned id;
            try { id = (unsigned)std::stoul(req.get_param_value("id")); }
            catch (...) { res.status = 400; return; }
            {   std::unique_lock<std::mutex> lock(_imut);
                if (_active > 0) _active--;
                touch();
            }
            std::cout << "client " << id << " bye" << std::endl;
        });

        _svr.Post("/write", [this](const httplib::Request& req, httplib::Response& res) {
            unsigned tag, index;
            try { tag = (unsigned)std::stoul(req.get_param_value("tag")); }
            catch (...) { res.status = 400; return; }
            {   std::unique_lock<std::mutex> lock(_imut); touch(); }
            _wal.write(index, tag, req.body);
            res.set_content(std::to_string(index), "text/plain");
        });

        _svr.Get("/read", [this](const httplib::Request& req, httplib::Response& res) {
            unsigned index, tag; std::string blob;
            try { index = (unsigned)std::stoul(req.get_param_value("index")); }
            catch (...) { res.status = 400; return; }
            {   std::unique_lock<std::mutex> lock(_imut); touch(); }
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
    // returns when there are no active clients
    // and no requests for `seconds` seconds
    void wait_idle(double seconds){
        for(;;){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::unique_lock<std::mutex> lock(_imut);
            double idle = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - _last).count();
            if (_active == 0 && idle >= seconds) return;
        }
    }
private:
    void touch(){ _last = std::chrono::steady_clock::now(); } 
private:
    wal& _wal;
    httplib::Server _svr;
    std::thread _thread;
    std::mutex _imut;
    unsigned _clients = 0, _active = 0;
    std::chrono::steady_clock::time_point _last;
};

// wal that forwards write() and read() to a remote stub over HTTP
class proxy : public wal {
public:
    proxy(const cfg& c):_addr(c._addr){
        std::string host; int port;
        split_addr(c._addr, host, port);
        _cli.reset(new httplib::Client(host, port));
        _cli->set_connection_timeout(5);
        auto res = _cli->Post("/hello", std::string(), "text/plain");
        if (!res || res->status != 200) fail("hello", res);
        _id = (unsigned)std::stoul(res->body);
    }
public:
    // client is done
    void bye(){
        std::unique_lock<std::mutex> lock(_mut);
        auto res = _cli->Post("/bye?id=" + std::to_string(_id), std::string(), "text/plain");
        if (!res || res->status != 200) fail("bye", res);
    }
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
    unsigned _id;
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
            cli.bye();
        }
        else if(_cfg._mode==cfg::type::srv){
            memwal wal;
            stub  srv(_cfg,wal);
            _run_cli_tasks(wal,task);
            // wait until the remote clients are done
            srv.wait_idle(10.0);
            wal.print(std::cout);
        }
        else if(_cfg._mode==cfg::type::cli){
            proxy cli(_cfg);
            _run_cli_tasks(cli,task);
            cli.bye();
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

int main(int argc, char* argv[])
{
    bool master = true;

    cfg cfg;
    env env(cfg);

    // usage: httpwal.exe [mem|one|srv|cli] [host:port]
    bool srv_mode = false;
    if (argc > 1) {
        std::string mode = argv[1];
        if      (mode == "mem") cfg.mode(cfg::type::mem);
        else if (mode == "one") cfg.mode(cfg::type::one);
        else if (mode == "cli") cfg.mode(cfg::type::cli);
        else if (mode == "srv") { cfg.mode(cfg::type::srv); srv_mode = true; }
        else { std::cerr << "usage: httpwal.exe [mem|one|srv|cli] [host:port]"; return EXIT_FAILURE; }
    }
    if (argc > 2) cfg.addr(argv[2]);

    //cfg.numtasks(cfg::SYSDEF);
    cfg.numtasks(10);

    env([&](unsigned pid,wal& wal) {
        unsigned index;
        wal.write(index,pid,std::string("blob"));
        env::delay(1.0);

        if (pid == 0 && master && !srv_mode){
            unsigned index=0; unsigned tag; std::string blob;
            while(wal.read(index, tag, blob)){
                std::cout << "index = " << index << " tag = " << tag << std::endl;
                index++;
            }
        }
    });
}
