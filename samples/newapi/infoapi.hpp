/*--------------------------------------------------------------------------*/
/*  Copyright 2025 Sergei Vostokin                                          */
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
#include "baseapi.hpp"

class Record{
protected:
    virtual void on_save(ostream&out){}
    virtual void on_load(istream&in) {}
};

class DatabaseWorker{
protected:
    DatabaseWorker(unsigned pid, EventLog&_log):log(_log),PID(pid){}
public:
    void run(int tag){
        //
    }
    bool set_io(bool){
        return false;
    }
    void sync(Record&){
        //
    }
protected:
    virtual void on_run(int tag)=0;
private:
    EventLog& log;
    unsigned PID;
};

#define IO_SECTION_BEGIN if(set_io(true)){
#define IO_SECTION_END   } set_io(false);
#define IO_SYNC_POINT(var)    sync(var);