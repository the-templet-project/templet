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

namespace templet {

	class client_side_wal : public write_ahead_log {
	public:
        
	};

    class server_side_wal : public write_ahead_log {
	public:
        server_side_wal(unsigned chunk_size, unsigned num_of_chunks, const std::string& wal_prefix, const std::string& wal_ext)
            : write_ahead_log(),
                _chunk_size(chunk_size), _num_of_chunks(num_of_chunks), _wal_prefix(wal_prefix), _wal_ext(wal_ext),
                _last_error(error::no_error), _wal_chunk_file(NULL), _is_chunk_full(false)
        {
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
            }
            _wal_chunk_file = fopen(_wal_chunk_file_name.c_str(), "ab");
                
            if(!_wal_chunk_file){
                std::cerr << "Cannot open WAL for appending: " << _wal_chunk_file_name; 
                exit(EXIT_FAILURE);
            }
        }

        ~server_side_wal(){ if(_wal_chunk_file) fclose(_wal_chunk_file);}
 
        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            
        }

        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            return false;
        }

        enum error {no_error};
        error last_error(){ return _last_error; }

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
            
            for(unsigned i=0; i < log.size(); i++){
                size_t ret_code;
                unsigned ubuf[3];//index,tag,size_buf
                
                ret_code = fread(ubuf, sizeof ubuf[0], 3, _wal_chunk_file);
                if(feof(_wal_chunk_file)) { _write_position = i; break;}
                
                if(ferror(_wal_chunk_file)){
                    std::cerr << "Error reading WAL (index,tag,size_buf): " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }

                if(i==0){ 
                    _base_position = ubuf[0];//index 
                    if(_base_position !=_chunk_size*chunk){
                        std::cerr << "Error (_base_position !=_chunk_size*chunk) in : " << _wal_chunk_file_name; 
                        exit(EXIT_FAILURE);
                    }
                }

                log[i].first = ubuf[1];//tag
                log[i].second.resize(ubuf[2]);
                ret_code = fread((void*)log[i].second.c_str(), sizeof(char), ubuf[2], _wal_chunk_file);//blob
                
                if(ret_code!=ubuf[2]||feof( _wal_chunk_file)||ferror( _wal_chunk_file)){
                    std::cerr << "Error reading WAL (blob): " << _wal_chunk_file_name; 
                    exit(EXIT_FAILURE);
                }
            }
            fclose(_wal_chunk_file);
            _wal_chunk_file = NULL;
        }
    private:
        unsigned    _base_position;
        unsigned    _write_position;
        bool        _is_chunk_full;
    private:
        unsigned    _chunk_size;
        unsigned    _num_of_chunks;
    private:
        unsigned    _wal_chunk;
        FILE*       _wal_chunk_file;
        std::string _wal_chunk_file_name;
    private:
        std::string _wal_prefix;
        unsigned    _size_of_chunk_field;
        std::string _wal_ext;
    private:
        error       _last_error;
	};

    class server_stub{
    public:
    };

    class client_proxy : public write_ahead_log {
    public:
    };
}
