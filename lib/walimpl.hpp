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
#include <stdio.h>
#include <stdlib.h>

#if (__cplusplus>=201703L)
#include <filesystem>
#endif

namespace templet {

    class server_side_wal : public write_ahead_log {
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
            // are set here:
            std::cout << "_base_position = " << _base_position << std::endl; 
            std::cout << "_write_position = " << _write_position << std::endl;
            std::cout << "_is_chunk_full = " << _is_chunk_full << std::endl;
            std::cout << "_wal_chunk = " << _wal_chunk << std::endl;
            std::cout << "_wal_chunk_file = " << _wal_chunk_file << std::endl;
            std::cout << "_wal_chunk_file_name = " << _wal_chunk_file_name << std::endl;
        }

        ~server_side_wal(){ if(_wal_chunk_file) fclose(_wal_chunk_file);}
 
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
			index = _write_position;

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
            
            if(_base_position <= index && index < _write_position){//in current chunk
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
                
                ret_code = fread(ubuf, sizeof ubuf[0], 3, _wal_chunk_file);
                if(ret_code==0 && feof(_wal_chunk_file)) break;

                if(ret_code!=3 && feof(_wal_chunk_file)){
                    if(_auto_repare){
                        fclose(_wal_chunk_file);
                        truncate_chunk(_wal_chunk_file_name,ret_code * sizeof ubuf[0]);
                        if(i==0) _base_position == _chunk_size*chunk; 
                        break;
                    }
                    else{
                        std::cerr << "Error read WAL (last record broken): " << _wal_chunk_file_name; 
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
                        truncate_chunk(_wal_chunk_file_name, 3*sizeof ubuf[0] + ret_code*sizeof(char));
                        break;
                    }
                    else{
                        std::cerr << "Error read WAL (last record broken): " << _wal_chunk_file_name; 
                        exit(EXIT_FAILURE);    
                    }
                }
                
                if(ferror( _wal_chunk_file)){
                    std::cerr << "Error read WAL (blob): " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }
            }
            _write_position = i;
            
            fclose(_wal_chunk_file);
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
        bool        _index_not_exist;
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
    };

    class client_proxy : public write_ahead_log {
    public:
    };
}
