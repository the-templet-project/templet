/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

namespace templet {
	
    class job {
	public:
		job(unsigned size):_size(size),_PID(0){}
        job():_size(0),_PID(0){}
        void init(unsigned size){_size = size;}
    public:
        inline void operator()(std::function<void(unsigned pid)> process){
            job::run(process);
        }
		void run(std::function<void(unsigned pid)> process){
            std::vector<std::thread> threads(_size);
            _beg=std::chrono::high_resolution_clock::now();
        	for (auto& t : threads) t = std::thread([&]{process(_PID++);}); 
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
        std::atomic_int _PID = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
	};

	class terminal {
	public:
		enum role{client,server};
		terminal(role, const char inpipe[], const char outpipe[]);
		terminal(role, const std::string& inpipe, const std::string& outpipe);
	public:
		void run(unsigned topic);
		void run(std::function<void(unsigned topic)> action);
	public:
		void write(const std::string&);
		void readln(std::string&);
	};

}
