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
#include <cstdio>

#if (__cplusplus>=201703L)
#include <filesystem>
#endif

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
            if(_size==0)_size=std::thread::hardware_concurrency();
            _beg=std::chrono::high_resolution_clock::now();
            if(_size==1) task(0);
            else{
                std::vector<std::thread> threads(_size);
            	for (auto& t : threads) t = std::thread([&]{task(_taskID++);}); 
                for (auto& t : threads) t.join();
            }
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
        void operator()(const char walfilename[],bool fix=true){
            _valid = false; _fixed = false;
            _size = 0; _first = 0;
            
            size_t ret_code;
            long last_valid_pos;
            unsigned current_index;
            
			unsigned ubuf[3];//index,tag,blob size
            std::string blob;

            FILE*file = fopen(walfilename, "rb");            
            if(!file){ fclose(file); return;}
                
            ret_code = fread(ubuf, 1, sizeof(ubuf), file);
            if(ret_code!=sizeof(ubuf)){ fclose(file); return;}
            if(ubuf[2]>blob.max_size()){ fclose(file); return;}
            
            blob.resize(ubuf[2]);//size
            
            ret_code = fread((void*)blob.c_str(), sizeof(char), ubuf[2], file);//blob
            if(ret_code!=ubuf[2]){ fclose(file); return;}

            last_valid_pos = ftell(file);
            _size = 1;
            _first = ubuf[0];
            current_index = _first + 1;
            
            for(;;){
    			ret_code = fread(ubuf, 1, sizeof(ubuf), file);

                if(ret_code==0 && feof(file)){ _valid=true; break;}
                
                if(ret_code!=sizeof(ubuf)) break;
                if(ubuf[2]>blob.max_size()) break;
                if(ubuf[0]!=current_index) break;
                
                blob.resize(ubuf[2]);//size
                
                ret_code = fread((void*)blob.c_str(), sizeof(char), ubuf[2], file);//blob
                if(ret_code!=ubuf[2]) break;
                
                last_valid_pos = ftell(file);
                _size++;
                current_index++;
            }
            fclose(file);
            
            if(_valid) return;

#if (__cplusplus>=201703L)
            if(fix){     
                std::filesystem::resize_file(walfilename, last_valid_pos);
                _fixed = true;
            }
#endif
        }
        void operator()(const std::string& walfilename,bool fix=true){
            operator()(walfilename.c_str(),fix);
        }
    public:
        bool valid(){return _valid;}
        bool fixed(){return _fixed;}
    public:
        unsigned size(){return ((_valid||_fixed)?_size:0);}
        unsigned first(){return ((_valid||_fixed)?_first:0);}
        unsigned last(){return ((_valid||_fixed)?_first+_size-1:0);}
    private:
        bool _valid;
        bool _fixed;
        unsigned _size;
        unsigned _first;
    };

}
