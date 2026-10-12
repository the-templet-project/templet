#pragma once

/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

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
            _numchunks(100000),_chunksize(10000),
            _addr("localhost:51717"),
            _waldir(""),_walpref("wal"),_walsuff(".dat"){}
    public:
        enum type {mem, one, srv, cli};
        static const unsigned SYSDEF = 0;
    public:
        void mode(type m){_mode=m;}
        void numtasks(unsigned nt){_numtasks=nt;}
        void addr(const std::string& a){_addr=a;}
        void numchunks(unsigned n){_numchunks=n;}
        void chunksize(unsigned s){_chunksize=s;}
        void waldir(const std::string& dir){_waldir=dir;}
        void walpref(const std::string& pref){_walpref=pref;}
        void walsuff(const std::string& suff){_walsuff=suff;}
    private:
        type _mode;
        unsigned _numtasks;
        std::string _addr;
        unsigned _numchunks;
        unsigned _chunksize;
        std::string _waldir;
        std::string _walpref;
        std::string _walsuff;
    };

    class interapi{};
    
    class stub{
    public:
        stub(const cfg&,interapi&){}
    };

    class srvwal : public interapi {
    public:
        srvwal(const cfg&){}
    };

    class proxy : public interapi{
    public:
        proxy(const cfg&){}
    };

    class cliwal : public wal {
    public:
        cliwal(const cfg&,interapi&){}
    public:
        void write(unsigned& index, unsigned tag, const std::string& blob) override {}
		bool read(unsigned index, unsigned& tag, std::string& blob) override {return false;}
    };
    
    class env {
    public:
        env(cfg&c):_cfg(c){}
    public:
        inline void operator()(std::function<void(unsigned taskID,templet::wal&wal)>task){env::run(task);}
        static void delay(double seconds){std::this_thread::sleep_for(std::chrono::duration<double>(seconds));}
        void run(std::function<void(unsigned taskID,templet::wal&wal)> task){
            if(_cfg._mode==cfg::type::mem){
                memwal wal;
                
                unsigned size=_cfg._numtasks;
                if(size==0)size=std::thread::hardware_concurrency();
                
                _beg=std::chrono::high_resolution_clock::now();
                
                if(size==1) task(0,wal);
                else{
                    std::vector<std::thread> threads(size);
                    for (auto& t : threads) t = std::thread([&]{
                        task(_taskID++,wal);
                    }); 
                    for (auto& t : threads) t.join();
                }
                _end=std::chrono::high_resolution_clock::now();
            }
            else if(_cfg._mode==cfg::type::one){
                srvwal srv(_cfg); 
                _run_cli_tasks(srv,task);
            }
            else if(_cfg._mode==cfg::type::srv){
                srvwal srv(_cfg); 
                templet::stub stub(_cfg,srv);
                _run_cli_tasks(srv,task);
            }
            else if(_cfg._mode==cfg::type::cli){
                proxy srv(_cfg);
                _run_cli_tasks(srv,task);
            }
        }
		double duration(){ 
            std::chrono::duration<double> dur = _end - _beg;
            return dur.count();
        }
    private:
        inline void _run_cli_tasks(interapi&srv,
            std::function<void(unsigned taskID,templet::wal&wal)> task)
        {
            unsigned size=_cfg._numtasks;
            if(size==0)size=std::thread::hardware_concurrency();
            
            _beg=std::chrono::high_resolution_clock::now();
            
            if(size==1){ 
                cliwal wal(_cfg,srv); 
                task(0,wal);
            }
            else{
                std::vector<std::thread> threads(size);
                for (auto& t : threads) t = std::thread([&]{
                    cliwal wal(_cfg,srv);task(_taskID++,wal);
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
}
