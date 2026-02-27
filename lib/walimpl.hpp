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
            : write_ahead_log(), _chunk_size(chunk_size), _num_of_chunks(num_of_chunks), _wal_prefix(wal_prefix), _wal_ext(wal_ext),
                _last_error(error::no_error), _wal_file(NULL), _overwrite(false){
                int mult;
                for(_size_of_chunk_field=0,mult=1; (num_of_chunks-1)/mult > 0; _size_of_chunk_field++, mult*=10);
            }

        void open(){
            unsigned cur_chunk = 0;
            bool has_chunk = false;
            
            std::cout << "_num_of_chunks = " << _num_of_chunks << std::endl;
            std::cout << "_size_of_chunk_field = " << _size_of_chunk_field << std::endl;
            
            for(cur_chunk = 0; cur_chunk < _num_of_chunks; cur_chunk++){
                std::ostringstream wal_name;
                
                wal_name << _wal_prefix; 
                if(_size_of_chunk_field) wal_name << std::setw(_size_of_chunk_field) << std::setfill('0') << cur_chunk; 
                wal_name << '.' <<_wal_ext;
                
                std::cout << "wal_name = " << wal_name.str() << std::endl;

                {
                    std::ifstream f(wal_name.str(),std::ios::binary);
                    if(f.fail()) break; else has_chunk = true;
                }
            }
            
            if(has_chunk){
                std::ostringstream wal_name;
                unsigned chunk = cur_chunk - 1;
                
                wal_name << _wal_prefix; 
                if(_size_of_chunk_field) wal_name << std::setw(_size_of_chunk_field) << std::setfill('0') << chunk; 
                wal_name << '.' <<_wal_ext;

                FILE* fp = fopen(wal_name.str().c_str(), "rb");
                if(!fp){
                    std::cerr << "Cannot open WAL: " << wal_name.str(); 
                    exit(EXIT_FAILURE);
                }

                log.resize(_chunk_size);
                
                for(unsigned i=0; i < log.size(); i++){
                    size_t ret_code;
                    
                    unsigned ubuf[3];//index,tag,size_buf
                    
                    ret_code = fread(ubuf, sizeof ubuf[0], 3, fp);
                    if(feof(fp)) { _index = i; break;}
                    
                    if(ferror(fp)){
                        std::cerr << "Error reading WAL (index,tag,size_buf): " << wal_name.str(); 
                        exit(EXIT_FAILURE);
                    }

                    if(i==0){ _base = ubuf[0]; assert(_base==_chunk_size*chunk); }

                    log[i].first = ubuf[1]; //tag
                    log[i].second.resize(ubuf[2]);
                    ret_code = fread((void*)log[i].second.c_str(), sizeof(char), ubuf[2], fp);//blob
                    
                    if(ret_code!=ubuf[2]||feof(fp)||ferror(fp)){
                        std::cerr << "Error reading WAL (blob): " << wal_name.str(); 
                        exit(EXIT_FAILURE);
                    }
                }
                fclose(fp);
            }
            //////////////////////////
            //////////////////////////
        }

        void write(unsigned& index, unsigned tag, const std::string& blob) override {
            
        }

        bool read(unsigned index, unsigned& tag, std::string& blob) override {
            return false;
        }
        enum error {no_error};
        error last_error(){ return _last_error; }
    private:
        unsigned    _base;
        unsigned    _index;
        bool        _overwrite;    
        FILE*       _wal_file;
        unsigned    _chunk_size;
        unsigned    _num_of_chunks;
        std::string _wal_prefix;
        unsigned    _size_of_chunk_field;
        std::string _wal_ext;
        std::string _wal_name;
        error       _last_error;
	};

    class server_stub{
    public:
    };

    class client_proxy : public write_ahead_log {
    public:
    };
}
