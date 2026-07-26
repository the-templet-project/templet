#pragma once

/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#include <functional>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>
#include <ostream>
#include <cassert>

namespace templet {
	
    class job {
	public:
		job(unsigned size):_size(size),_taskID(0){}
        job():_size(std::thread::hardware_concurrency()),_taskID(0){}
        void init(unsigned size){_size = size;}
    public:
        inline void operator()(std::function<void(unsigned taskID)> task){
            job::run(task);
        }
		void run(std::function<void(unsigned taskID)> task){
            std::vector<std::thread> threads(_size);
            _beg=std::chrono::high_resolution_clock::now();
        	for (auto& t : threads) t = std::thread([&]{task(_taskID++);}); 
            for (auto& t : threads) t.join();
            _end=std::chrono::high_resolution_clock::now();
        }
		static void delay(double seconds){
            std::this_thread::sleep_for(
                std::chrono::duration<double>(seconds));
        }
		double duration(){ 
            std::chrono::duration<double> dur = _end - _beg;
            return dur.count();
        }
    private:
        unsigned _size;
        std::atomic_int _taskID = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
	};

	class walfix {
    public:
        walfix(bool autofix=true):_autofix(autofix){}
    public:
        bool operator()(const char walfilename[]){
            ////
            return _valid;
        }
        bool operator()(const std::string& walfilename){
            return operator()(walfilename.c_str());
        }
    public:
        bool valid(){return _valid;}
        bool fixed(){return _fixed;}
    public:
        unsigned size(){return (_valid?_size:0);}
        unsigned first(){return (_valid?_first:0);}
        unsigned last(){return (_valid?_first+_size-1:0);}
    private:
        bool _valid;
        bool _fixed;
        unsigned _size;
        unsigned _first;
        bool _autofix;
    };

}
