/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/
#pragma once

#include "syncmem.hpp"

#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <thread>
#include <atomic>

#if (__cplusplus>=201703L)
#include <filesystem>
#endif

namespace templet {

	class write_ahead_log {
	public:
        virtual ~write_ahead_log(){}
		virtual void write(unsigned& index, unsigned tag, const std::string& blob) {
			std::unique_lock<std::mutex> lock(mut);
			log.push_back(std::pair<unsigned, std::string>(tag, blob));
			index = log.size() - 1;
		}
		virtual bool read(unsigned index, unsigned& tag, std::string& blob) {
			std::unique_lock<std::mutex> lock(mut);
			if (index < log.size()) { tag = log[index].first; blob = log[index].second; return true; }
			return false;
		}
	protected:
		std::vector<std::pair<unsigned, std::string>> log;
		std::mutex mut;
	};

    class in_file_wal : public write_ahead_log {
    public:
        in_file_wal(const std::string&file):in_file_wal(file,true,true){}
        in_file_wal(const std::string&file,bool resume,bool lazy):
            _file(file),_resume(resume),_lazy(lazy),_hfile(NULL){
            
            if(!_resume){ // new calcs         
                _hfile = fopen(_file.c_str(), "rb");
                
                if(_hfile) { std::cerr << "the file '" << _file
                    << "' already exists; data loss is possible"; fclose(_hfile);
                    exit(EXIT_FAILURE);
                }
            }
            else{ // resume calcs
                _hfile = fopen(_file.c_str(), "rb");

                if(!_hfile){ std::cerr << "cannot open the file '" << _file
                    << "' for reading"; 
                    exit(EXIT_FAILURE);
                }
                
                unsigned ubuf[3]; //index,tag,blob size
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
                fclose(_hfile);
            }
            _hfile = fopen(_file.c_str(), "ab");

            if(!_hfile){ std::cerr << "cannot open the file '" << _file
                << "' for appending"; 
                exit(EXIT_FAILURE);
            }        
        }
        virtual ~in_file_wal() override {fclose(_hfile);}

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
        bool _resume;
        bool _lazy;
    };

    class server_side_wal : public write_ahead_log {
    friend class client_side_wal;
	public:
        server_side_wal(unsigned chunk_size, unsigned num_of_chunks,
                        const std::string& wal_prefix, const std::string& wal_ext,
                        bool lasy_save=false, bool auto_repare=false): write_ahead_log(),
                _lasy_save(lasy_save), _auto_repare(auto_repare),
                _chunk_size(chunk_size), _num_of_chunks(num_of_chunks), _wal_prefix(wal_prefix), _wal_ext(wal_ext),
                _index_not_exist(true), _wal_chunk_file(NULL), _is_chunk_full(false)
        {
#if (__cplusplus<201703L)
            _auto_repare = false;
#endif   
            _size_of_chunk_field=0;
            for(int mult=1; (num_of_chunks-1)/mult > 0; _size_of_chunk_field++, mult*=10);

            if(find_last_not_empty_chunk(_wal_chunk))
                load_chunk(_wal_chunk);
            else{
                _wal_chunk = 0;
                {
                    std::ostringstream chunk_file_name;
                    chunk_file_name << _wal_prefix; 
                    if(_size_of_chunk_field) chunk_file_name << std::setw(_size_of_chunk_field) << std::setfill('0') << _wal_chunk; 
                    chunk_file_name << '.' <<_wal_ext;
            
                    _wal_chunk_file_name = chunk_file_name.str();
                }
                log.resize(_chunk_size);
                _base_position  = 0;
                _write_position = 0;
            }
            _wal_chunk_file = fopen(_wal_chunk_file_name.c_str(), "ab");
                
            if(!_wal_chunk_file){
                std::cerr << "Cannot open WAL for appending: " << _wal_chunk_file_name; 
                exit(EXIT_FAILURE);
            }
        }

        virtual ~server_side_wal() override { if(_wal_chunk_file) fclose(_wal_chunk_file);}
 
        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            std::unique_lock<std::mutex> lock(mut);

            if(_write_position == _chunk_size){
                fclose(_wal_chunk_file);
                _wal_chunk_file = NULL;
                
                _wal_chunk++;
                if(_wal_chunk == _num_of_chunks){
                    std::cerr << "The last chunk is full: " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }
                {
                    std::ostringstream chunk_file_name;
                    chunk_file_name << _wal_prefix; 
                    if(_size_of_chunk_field) chunk_file_name << std::setw(_size_of_chunk_field) << std::setfill('0') << _wal_chunk; 
                    chunk_file_name << '.' <<_wal_ext;
                    _wal_chunk_file_name = chunk_file_name.str();
                }
                
                _wal_chunk_file = fopen(_wal_chunk_file_name.c_str(), "ab");
                if(!_wal_chunk_file){
                    std::cerr << "Cannot open WAL for appending: " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }
                
                _base_position = _chunk_size*_wal_chunk;
                _write_position = 0;
                _is_chunk_full = true;
            }
            
			log[_write_position].first  = tag;
            log[_write_position].second = blob;
			index = _write_position + _base_position;

            {
                size_t ret_code;
                unsigned ubuf[3];//index,tag,blob size
                
                ubuf[0] = index; //index
                ubuf[1] = tag;   //tag
                ubuf[2] = blob.size(); //blob size

                ret_code = fwrite(ubuf, sizeof ubuf[0], 3, _wal_chunk_file);
                if(ret_code !=3){
                    std::cerr << "Error write WAL (index,tag,size): " << _wal_chunk_file_name;  
                    exit(EXIT_FAILURE);
                }

                ret_code = fwrite(blob.c_str(), sizeof(char), ubuf[2], _wal_chunk_file);
                if(ret_code != ubuf[2]){
                    std::cerr << "Error write WAL (blob): " << _wal_chunk_file_name;  
                    exit(EXIT_FAILURE);
                }
                
                if(!_lasy_save) fflush(_wal_chunk_file);
            }
            _write_position++;
        }

        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            std::unique_lock<std::mutex> lock(mut);
            
            if((_base_position+_write_position) <= index){
                _index_not_exist = true;
                return false;
            }
            
            if(_base_position <= index && index < _base_position+_write_position){//in current chunk
                tag = log[index-_base_position].first; 
                blob = log[index-_base_position].second;
                _index_not_exist = false;
                return true;
            }

            if(!_is_chunk_full){
                _index_not_exist = false;
                return false;
            }
            
        	if((_base_position-_chunk_size)+_write_position <= index){//in previous chunk
                 tag = log[index-(_base_position-_chunk_size)].first; 
                blob = log[index-(_base_position-_chunk_size)].second;
                _index_not_exist = false;
                return true;
            }
            
            _index_not_exist = false;
			return false;
        }

        bool index_not_exist(){ return _index_not_exist; }

    private:
        bool find_last_not_empty_chunk(unsigned& not_empty_chunk){
            unsigned cur_chunk = 0;
            bool has_not_empty_chunk = false;
            
            for(cur_chunk = 0; cur_chunk < _num_of_chunks; cur_chunk++){
                std::ostringstream wal_name;      
                wal_name << _wal_prefix; 
                if(_size_of_chunk_field) wal_name << std::setw(_size_of_chunk_field) << std::setfill('0') << cur_chunk; 
                wal_name << '.' <<_wal_ext;            
                {
                    std::ifstream f(wal_name.str(),std::ios::binary);
                    if(f.fail()) break; else has_not_empty_chunk = true;
                }
            }
            if(has_not_empty_chunk) not_empty_chunk = cur_chunk - 1;
            return has_not_empty_chunk;
        }

        void load_chunk(unsigned chunk){
            {
                std::ostringstream chunk_file_name;
                chunk_file_name << _wal_prefix; 
                if(_size_of_chunk_field) chunk_file_name << std::setw(_size_of_chunk_field) << std::setfill('0') << chunk; 
                chunk_file_name << '.' <<_wal_ext;
        
                _wal_chunk_file_name = chunk_file_name.str();
            }
            _wal_chunk_file = fopen(_wal_chunk_file_name.c_str(), "rb");
                
            if(!_wal_chunk_file){
                std::cerr << "Cannot open WAL for reading: " << _wal_chunk_file_name; 
                exit(EXIT_FAILURE);
            }

            log.resize(_chunk_size);

            unsigned i;
            for(i=0; i < _chunk_size; i++){
                size_t ret_code;
                unsigned ubuf[3];//index,tag,blob size
                
                ret_code = fread(ubuf, 1, sizeof ubuf, _wal_chunk_file);
                if(ret_code==0 && feof(_wal_chunk_file)) break;

                if(ret_code!=3 && feof(_wal_chunk_file)){
                    if(_auto_repare){
                        fclose(_wal_chunk_file);
                        _wal_chunk_file = NULL;
                        truncate_chunk(_wal_chunk_file_name,ret_code);
                        if(i==0) _base_position = _chunk_size*chunk; 
                        break;
                    }
                    else{
                        std::cerr << "Error read WAL (last record broken in header): " << _wal_chunk_file_name; 
                        exit(EXIT_FAILURE);    
                    }
                }
                
                if(ferror(_wal_chunk_file)){
                    std::cerr << "Error read WAL (index,tag,blob size): " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }

                if(i==0){ 
                    _base_position = ubuf[0];//index 
                    if(_base_position !=_chunk_size*chunk){
                        std::cerr << "Error (_base_position !=_chunk_size*chunk) in : " << _wal_chunk_file_name; 
                        exit(EXIT_FAILURE);
                    }
                }
                else{
                    if(ubuf[0] != _base_position + i){
                        std::cerr << "Error (index != _base_position + i) in : " << _wal_chunk_file_name; 
                        exit(EXIT_FAILURE);
                    }
                }

                log[i].first = ubuf[1];//tag
                log[i].second.resize(ubuf[2]);//blob size
                ret_code = fread((void*)log[i].second.c_str(), sizeof(char), ubuf[2], _wal_chunk_file);//blob

                if(ret_code!=ubuf[2] && feof( _wal_chunk_file)){
                    if(_auto_repare){
                        fclose(_wal_chunk_file);
                        _wal_chunk_file = NULL;
                        truncate_chunk(_wal_chunk_file_name, 3*(sizeof ubuf[0]) + ret_code*sizeof(char));
                        break;
                    }
                    else{
                        std::cerr << "Error read WAL (last record broken in blob): " << _wal_chunk_file_name; 
                        exit(EXIT_FAILURE);    
                    }
                }
                
                if(ferror( _wal_chunk_file)){
                    std::cerr << "Error read WAL (blob): " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }
            }
            _write_position = i;
            
            if(_wal_chunk_file)fclose(_wal_chunk_file);
            _wal_chunk_file = NULL;
        }

        void truncate_chunk(const std::string& filename,unsigned n){
#if (__cplusplus>=201703L)
            std::filesystem::path p=filename;
            std::filesystem::resize_file(p,std::filesystem::file_size(p)-n);
#endif
        }

    private:
        unsigned    _base_position;
        unsigned    _write_position;
        bool        _is_chunk_full;
    private:
        unsigned    _wal_chunk;
        FILE*       _wal_chunk_file;
        std::string _wal_chunk_file_name;
    private:
        unsigned    _chunk_size;
        unsigned    _num_of_chunks;
    private:
        bool        _lasy_save;
        bool        _auto_repare;
    private:
        std::string _wal_prefix;
        unsigned    _size_of_chunk_field;
        std::string _wal_ext;
    private:
        bool _index_not_exist;
	};

	class client_side_wal : public write_ahead_log {
	public:
        client_side_wal(unsigned chunk_size, unsigned num_of_chunks, 
                        const std::string& wal_prefix, const std::string& wal_ext, server_side_wal& server_wal):
            _chunk_size(chunk_size), _num_of_chunks(num_of_chunks),
            _wal_prefix(wal_prefix), _wal_ext(wal_ext), _server_wal(server_wal),
            _is_chunk_full(false)
        {
            log.resize(chunk_size);
            _size_of_chunk_field=0;
            for(int mult=1; (num_of_chunks-1)/mult > 0; _size_of_chunk_field++, mult*=10);
        }

        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            std::unique_lock<std::mutex> lock(mut);
            _server_wal.write(index,tag,blob);
        }
        
        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            std::unique_lock<std::mutex> lock(mut);

            if(_is_chunk_full && (_base_position <= index && index < _base_position+_chunk_size)){
                tag = log[index-_base_position].first; 
                blob = log[index-_base_position].second;
                return true;    
            }

            if(_server_wal.read(index,tag,blob)) return true;
            if(_server_wal.index_not_exist()) return false;

            load_chunk_by_index(index);

            tag = log[index-_base_position].first; 
            blob = log[index-_base_position].second;
            
            return true;
        }

    bool index_not_exist(){ return _server_wal._index_not_exist; }

    private:
        void load_chunk_by_index(unsigned index){ 
            std::string wal_chunk_file_name;
            FILE* wal_chunk_file;
            {
                std::ostringstream chunk_file_name;
                unsigned chunk = index / _chunk_size;
                    
                chunk_file_name << _wal_prefix; 
                if(_size_of_chunk_field) chunk_file_name << std::setw(_size_of_chunk_field) << std::setfill('0') << chunk; 
                chunk_file_name << '.' <<_wal_ext;
        
                wal_chunk_file_name = chunk_file_name.str();
                wal_chunk_file = fopen(wal_chunk_file_name.c_str(), "rb");
                
                _base_position =_chunk_size*chunk;
            }
                
            if(!wal_chunk_file){
                std::cerr << "Cannot open WAL for reading: " << wal_chunk_file_name; 
                exit(EXIT_FAILURE);
            }
            
            for(unsigned i=0; i < _chunk_size; i++){
                size_t ret_code;
                unsigned ubuf[3];//index,tag,blob size
                
                ret_code = fread(ubuf, sizeof ubuf[0], 3, wal_chunk_file);

                if(ret_code!=3){
                    std::cerr << "Error read WAL (index,tag,blob size): " << wal_chunk_file_name;  
                    exit(EXIT_FAILURE);    
                }

                if(_base_position+i != ubuf[0]){
                    std::cerr << "Error (index != _base_position + i) in : " << wal_chunk_file_name;
                    exit(EXIT_FAILURE);
                }
                    
                log[i].first = ubuf[1];//tag
                log[i].second.resize(ubuf[2]);//blob size
                ret_code = fread((void*)log[i].second.c_str(), sizeof(char), ubuf[2], wal_chunk_file);//blob

                if(ret_code!=ubuf[2]){
                    std::cerr << "Error read WAL (blob): " << wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }
            }
            fclose(wal_chunk_file);
            _is_chunk_full = true;
        }

    private:
        unsigned    _base_position;
        bool        _is_chunk_full;
    private:
        unsigned    _chunk_size;
        unsigned    _num_of_chunks;
    private:
        std::string _wal_prefix;
        unsigned    _size_of_chunk_field;
        std::string _wal_ext;
    private:
        server_side_wal& _server_wal;
	};

    class server_stub{
    public:
        server_stub(write_ahead_log*,std::string& addr,unsigned port){}
    public:
        void run(){}
        void stop(){}
    };

    class client_proxy : public write_ahead_log {
    public:
        client_proxy(std::string& addr,unsigned port){}
    };

    class config{
    friend class environment;
    public:
        void server(const std::string& a,unsigned p){_server=true;_addr=a;_port=p;}
        void server(bool s=true){_server=s;}
        void client(const std::string& a,unsigned p){_client=true;_addr=a;_port=p;}
        void client(bool c=true){_client=c;}
        void poolsize(unsigned s){_poolsize=s;}
        void file(const std::string& f){_file=f;_memory=false;}
        void file(bool f){_memory=!f;}
        void resume(bool r=true){_resume=r;}
        void lazy(bool l=true){_lazy=l;}
    private:
        bool _memory=true;
        bool _server=false;
        bool _client=false;
        std::string _addr="localhost:51717";
        unsigned _port=51717;
        unsigned _poolsize=0;
        std::string _file="wal.dat";
        bool _resume=false;
        bool _lazy=true;
    };

    class environment{
    public:
        environment(config&cfg):_conf(cfg){}
        ~environment(){ close(); }    
    public:
        inline void operator()(std::function<void(unsigned taskID,write_ahead_log&wal)>task){environment::run(task);}
        
        void run(std::function<void(unsigned taskID,write_ahead_log&)> task){
            if(_conf._server){
                _srv_wal = _conf._memory ? new write_ahead_log : new in_file_wal(_conf._file,_conf._resume,_conf._lazy);
                _srv_stub = new server_stub(_srv_wal,_conf._addr,_conf._port);
                _srv_stub->run();
            }
            else{
                _srv_wal = nullptr;
                _srv_stub = nullptr;
            }
            
            _cli_wal = _conf._client ? new client_proxy(_conf._addr,_conf._port) : nullptr;
            _cli_wal = (!_conf._client && !_conf._server)?
                (_conf._memory ? new write_ahead_log : new in_file_wal(_conf._file,_conf._resume,_conf._lazy)) : _cli_wal;
            
            if(_conf._server && !_conf._client) 
                run_clients(_srv_wal,task);
            else
                run_clients(_cli_wal,task);
            
            if(_conf._server) _srv_stub->stop();
            close();
        }
    private:
        void run_clients(write_ahead_log*wal,std::function<void(unsigned taskID,write_ahead_log&)> task){
            if(_conf._poolsize==1)
                 task(0,*wal);
            else{
                std::vector<std::thread> threads(
                        _conf._poolsize==0 ? std::thread::hardware_concurrency() : _conf._poolsize);
                std::atomic_int _taskID = 0;
                for (auto& t : threads) t = std::thread([&]{task(_taskID++,*wal);}); 
                for (auto& t : threads) t.join();
            }
        }
        void close(){
            if(_cli_wal) { delete _cli_wal; _cli_wal = nullptr; }
            if(_srv_wal) { delete _srv_wal; _srv_wal = nullptr; } 
            if(_srv_stub){ delete _srv_stub; _srv_stub = nullptr; } 
        }
    private:
        config& _conf;
        write_ahead_log *_srv_wal=nullptr;
        server_stub* _srv_stub=nullptr;
        write_ahead_log *_cli_wal=nullptr;
    };
}
