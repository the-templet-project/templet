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
#include <vector>
#include <iostream>
#include "baseapi.hpp"

class Transaction;

class DataObject{
friend class Transaction;
public:
    DataObject(unsigned pid, EventLog&_log):log(_log),PID(pid),next_to_read_event(0){}
    void update();
private:
    unsigned register_transaction(Transaction*tr){
        unsigned tag = transactions.size();
        transactions.push_back(tr);
        return tag;
    }
private:
    EventLog& log;
    unsigned PID;
private:
    vector<Transaction*> transactions;
    unsigned next_to_read_event;
};

class Transaction{
friend class DataObject;
public:
    Transaction(DataObject&obj):data_object(obj){tag = obj.register_transaction(this);}
    void run(){
        BLOB blob; ostringstream out;
        
        on_save(out);
        blob = out.rdbuf()->str();
       
        data_object.log.write(tag,data_object.PID,blob);
        data_object.update();
    }
protected:
    virtual void on_run()=0;
    virtual void on_save(ostream&out)=0;
    virtual void on_load(istream&in)=0;
private:
    unsigned tag;
    DataObject& data_object;
};

void DataObject::update(){
    for(;;){
        unsigned tag; unsigned pid; BLOB blob;
        if(!log.read(next_to_read_event,tag,pid,blob))return;

        Transaction* next_transaction = transactions[tag];

        istringstream in;
        in.rdbuf()->str(blob);
        
        next_transaction->on_load(in);
        next_transaction->on_run();
        
        next_to_read_event++;
    }
}
