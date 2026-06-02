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

namespace templet {
	
    class job {
	public:
		job(unsigned size):_size(size),_taskID(0){}
        job():_size(0),_taskID(0){}
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

	class terminal {
	public:
		enum role{client,server};
		terminal(role, const char inpipe[], const char outpipe[]);
		terminal(role, const std::string& inpipe, const std::string& outpipe);
	public:
		void run(unsigned topic);// client side
		void run(std::function<void(unsigned topic)> action);// server side
	public:// server side
		void write(const std::string&);
		void readln(std::string&);
		void readln(std::ostream&);
	};

}
