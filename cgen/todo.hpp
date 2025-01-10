#include <map>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <condition_variable>
#include <cassert>
#include <limits>

using namespace std;

struct BLOB{unsigned size;void*buf;};

class EventLog{
public:
    bool try_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob){
        unique_lock lk(mut);
        map<unsigned,EVENT>::iterator it; 
        if((it=events.find(ord))!=events.end()){
            tag = it->second.tag; pid = it->second.PID; blob = it->second.blob;
            return true;
        }
        return false;
    }
    void wait_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob){
        unique_lock lk(mut);
        map<unsigned,EVENT>::iterator it; 
        while((it=events.find(ord))==events.end()){cv.wait(lk);}
        tag = it->second.tag; pid = it->second.PID; blob = it->second.blob;  
    }
    void write(unsigned tag,unsigned&pid,BLOB blob){
        unique_lock lk(mut);
        EVENT ev{tag,pid,blob};
        events[next_add_event++] = ev;
        cv.notify_all();
    }
private:
    struct EVENT{ unsigned tag; unsigned PID; BLOB blob; };
    map<unsigned,EVENT> events;
    unsigned next_add_event;
    mutex mut;
    condition_variable cv;
};

/////////////////////////////////////////////////////////////////////

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
    virtual bool on_read(unsigned ord,unsigned tag,unsigned pid,BLOB blob)=0;
    virtual bool on_write(unsigned&tag,BLOB&blob)=0;
private:
    EventLog& log;
    unsigned current_event;
    unsigned PID;
};

class InterfaceWorker{
public:
    InterfaceWorker(unsigned pid, EventLog&_log):log(_log),PID(pid),tag(0){}
public:
    void query(BLOB in,BLOB&out){
        unique_lock lk(mut);
        unsigned query_tag=tag++;
        log.write(query_tag,PID,in);
        map<unsigned,BLOB>::iterator it;
        while((it=answers.find(query_tag))==answers.end()){cv.wait(lk);}
        out = it->second;
        answers.erase(it);
    }
    void notify(unsigned tag,BLOB blob){ log.write(tag,PID,blob); }
public:
    void answer(unsigned tag,BLOB blob){
        unique_lock lk(mut);
        answers[tag]=blob;
        cv.notify_all();
    }
private:
    EventLog& log;
    unsigned PID;
    unsigned tag;
    mutex mut;
    condition_variable cv;
    map<unsigned,BLOB> answers;
};

/////////////////////////////////////////////////////////////////////

class TaskEngine;

class Task{
friend class TaskEngine;
public:
    Task(TaskEngine&);
public:
    void submit();
    bool access(){ return !submitted; }
protected:
    virtual void on_run(){}
    virtual void on_continue(){}
protected:
    virtual void on_save(BLOB&){}
    virtual void on_load(BLOB){}
private:
    TaskEngine& taskeng;
    bool submitted;
    unsigned ID;
};

class TaskEngine: public ComputeWorker{
friend class Task;
public:
    TaskEngine(unsigned pid,EventLog& log):ComputeWorker(pid,log){}
private:
    bool on_read(unsigned,unsigned tag,unsigned,BLOB blob) override{
        
        if(tag==numeric_limits<unsigned>::max()) return false;
        
        Task* task = tasks.at(tag);
        to_be_selected_for_exec.erase(tag);
        
        assert(task->submitted);
        task->on_load(blob);
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
        to_be_selected_for_exec.erase(it);
        
        task->on_run();
        task->on_save(blob);
        tag = task->ID;
       
        return true;
    }
private:
    void add_task(Task*t){ t->ID = tasks.size(); tasks.push_back(t); }
    void submit(Task*t){ to_be_selected_for_exec[t->ID] = t; }
private:
    map<unsigned,Task*> to_be_selected_for_exec;
    vector<Task*> tasks;
};

Task::Task(TaskEngine&te):taskeng(te),submitted(false)
{
    te.add_task(this);
}

void Task::submit(){
    assert(!submitted);
    submitted = true;
    taskeng.submit(this);
}

/////////////////////////////////////////////////////////////////////

template<typename T>
class MasterWorkers{
public:
    MasterWorkers(unsigned num_workers){
        /////////////////////////
    }
protected:
    virtual bool on_get(T&)=0;
    virtual void on_run(T&)=0;
    virtual void on_put(T&)=0;
protected:
    virtual void on_save(const T&,BLOB&){}
    virtual void on_load(T&,const BLOB&){}
private:
    struct WorkerTask:public Task{
        WorkerTask(TaskEngine&eng,MasterWorkers&m):Task(eng),mw(m){}
        T task; MasterWorkers& mw;
        void on_run() override{ mw.on_run(*this); }
        void on_continue() override{
            ////////////////
        }
        void on_save(BLOB&blob) override{ mw.on_save(*this,blob); }
        void on_load(BLOB blob) override{ mw.on_load(*this,blob); }
    };
};

/////////////////////////////////////////////////////////////////////

class DatabaseWorker{
protected:
    DatabaseWorker(unsigned pid, EventLog&_log):log(_log),PID(pid){}
public:
    void run(unsigned tag){
        ///
    }
protected:
    bool once(){
        return false;
    }
    void check(const string&){
        ///
    }
protected:
    virtual void on_run(unsigned tag)=0;
private:
    EventLog& log;
    unsigned PID;
};
