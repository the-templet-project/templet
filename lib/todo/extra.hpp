/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

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

	class walfixer {
	public:
		walfixer(const char filename[]): walfixer(std::string(filename)) {}
		walfixer() : walfixer(std::string()) {}
		walfixer(const std::string& filename);
	public:
		bool check();
		bool check(const char filename[]) { return check(std::string(filename)); }
		bool check(const std::string& filename);
	public:
		bool corrupted() { assert(checked() && "walfix: wrong call pattern"); return false; }
		bool checked();
		unsigned size();
		unsigned first();
		unsigned last();
	public:
		bool fix();
	public:
		void print(std::ostream&);
		void print(std::ostream&,unsigned by_ord_from, unsigned by_ord_to);
		void print(std::ostream&, unsigned by_rev_ord);
	};

}
