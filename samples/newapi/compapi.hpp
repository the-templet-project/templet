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

#include <cassert>
#include <sstream>
#include <sstream>
#include <list>

class ComputeWorker{
protected:
    ComputeWorker(unsigned pid, EventLog&_log):log(_log),PID(pid),current_event(0){}
public:
    void run(){
        for(;;){
            unsigned tag; unsigned pid; BLOB blob;
            
            log.wait_read(current_event,tag,pid,blob);
            on_read(current_event,tag,pid,blob);
            current_event++;
            
            while(log.try_read(current_event,tag,pid,blob)){
                if(!on_read(current_event,tag,pid,blob))return;
                current_event++;
            }

            if(on_write(tag,blob))log.write(tag,PID,blob);
        } 
    }
protected:
    virtual bool on_read(unsigned ord,unsigned tag,unsigned pid,const BLOB&blob)=0;
    virtual bool on_write(unsigned&tag,BLOB&blob)=0;
private:
    EventLog& log;
    unsigned current_event;
    unsigned PID;
};

class TaskEngine;

class Task{
friend class TaskEngine;
public:
    Task(TaskEngine&te):taskeng(te),submitted(false){}
public:
    void submit();
    bool access(){ return !submitted; }
protected:
    virtual void on_run(){}
    virtual void on_continue(){}
protected:
    virtual void on_save(ostream&){}
    virtual void on_load(istream&){}
private:
    TaskEngine& taskeng;
    bool submitted;
};

class TaskEngine: public ComputeWorker{
friend class Task;
public:
    TaskEngine(unsigned pid,EventLog& log):
        ComputeWorker(pid,log),cur_submitted_task_tag(0){}
private:
    bool on_read(unsigned,unsigned tag,unsigned,const BLOB&blob) override{
        if(tag==numeric_limits<unsigned>::max())return false;
        
        Task* task;
        if(to_be_selected_for_exec.find(tag)!=to_be_selected_for_exec.end()){      
            task = to_be_selected_for_exec[tag];
            assert(task->submitted);
            to_be_selected_for_exec.erase(tag);
        }    
        else if(selected_for_exec.find(tag)!=selected_for_exec.end()){
            task = selected_for_exec[tag];
            assert(task->submitted);
            selected_for_exec.erase(tag);
        }
        else
            return true;
        
        istringstream in;
        in.rdbuf()->str(blob);
        task->on_load(in);
        
        task->submitted = false;
        task->on_continue();
        
        return true;
    }
    bool on_write(unsigned&tag,BLOB&blob) override{
        
        unsigned size = to_be_selected_for_exec.size(); 
        if(!size){ tag = numeric_limits<unsigned>::max(); return true; }
        
        unsigned selected = rand()%size;

        map<unsigned,Task*>::iterator it; int i;
        for(it=to_be_selected_for_exec.begin(),i=0;i!=selected;i++,it++);
        
        Task* task = it->second;
        tag = it->first;
        to_be_selected_for_exec.erase(it);
        
        task->on_run();

        ostringstream out;
        task->on_save(out);
        blob = out.rdbuf()->str();

        selected_for_exec[tag] = task;
        
        return true;
    }
private:
    void submit(Task*t){ to_be_selected_for_exec[cur_submitted_task_tag++] = t; }
private:
    map<unsigned,Task*> to_be_selected_for_exec;
    map<unsigned,Task*> selected_for_exec;
    unsigned cur_submitted_task_tag;
};

void Task::submit(){ assert(!submitted); submitted = true; taskeng.submit(this); }

template<typename T>
class MasterWorkers{
public:
    MasterWorkers(unsigned num_workers,unsigned pid,EventLog& log):task_eng(pid,log){
        for(int i=0;i<num_workers;i++)
            ready.push_back(new Worker(task_eng,*this));
    }
    ~MasterWorkers(){ for(auto i: ready) delete i; }
    void run(){ submit_tasks(); task_eng.run(); }
protected:
    virtual bool on_get(T&)=0;
    virtual void on_run(T&)=0;
    virtual void on_put(T&)=0;
protected:
    virtual void on_save(const T&,ostream&){}
    virtual void on_load(T&,istream&){}
private:
    struct Worker:public Task{
        Worker(TaskEngine&eng,MasterWorkers&m):Task(eng),mw(m){}
        T task; MasterWorkers& mw;
        void on_run() override{ mw.on_run(*this); }
        void on_continue() override{
            mw.on_put(task); mw.ready.push_back(*this); mw.submit_tasks();
        }
        void on_save(ostream&out) override{ mw.on_save(*this,out); }
        void on_load(istream&in) override{ mw.on_load(*this,in); }
    };
private:
    void submit_tasks(){
        typename list<Worker*>::iterator it;
        while( ((it=ready.begin())!=ready.end()) && on_get(it) ){ 
            it.submit(); ready.erase(it); 
        }
    }
private:
    list<Worker*> ready;
    TaskEngine task_eng;
};
