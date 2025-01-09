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
    unsigned write(unsigned tag,unsigned&pid,BLOB blob){
        unsigned ord;
        unique_lock lk(mut);
        EVENT ev{tag,pid,blob};
        ord = next_add_event;
        events[next_add_event++] = ev;
        cv.notify_all();
        return ord;
    }
private:
    struct EVENT{ unsigned tag; unsigned PID; BLOB blob; };
    map<unsigned,EVENT> events;
    unsigned next_add_event;
    mutex mut;
    condition_variable cv;
};

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
    InterfaceWorker(unsigned pid, EventLog&_log):log(_log),PID(pid){}
public:
    void query(unsigned tag,BLOB out,BLOB&in){
        unique_lock lk(mut);
        unsigned ord = log.write(tag,PID,out);
        map<unsigned,BLOB>::iterator it;
        while((it=answers.find(ord))==answers.end()){cv.wait(lk);}
        in = it->second;
    }
    void notify(unsigned tag,BLOB blob){ log.write(tag,PID,blob); }
public:
    void answer(unsigned ord,BLOB blob){
        unique_lock lk(mut);
        answers[ord]=blob;
        cv.notify_all();
    }
private:
    EventLog& log;
    unsigned PID;
    mutex mut;
    condition_variable cv;
    map<unsigned,BLOB> answers;
};

class TaskEngine;

class Task{
friend class TaskEngine;
public:
    Task(TaskEngine&);
public:
    bool submit();
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
    TaskEngine(unsigned pid,EventLog& log):ComputeWorker(pid,log),num_submitted(0){}
private:
    bool on_read(unsigned,unsigned tag,unsigned,BLOB blob) override{
        if(tag==numeric_limits<unsigned>::max()) return false;
        
        assert(tag < tasks.size());
        Task* task = tasks[tag];
        
        assert(task->submitted);
        task->on_load(blob);
        task->on_continue();
        task->submitted = false;
        num_submitted--;
        
        return true;
    }
    bool on_write(unsigned&tag,BLOB&blob) override{  
        if(num_submitted==0){ 
            tag = numeric_limits<unsigned>::max();
            return true;
        }
        
        unsigned size = planned.size();
        unsigned selected = rand()%size;
        Task* task = planned[selected];
        
        task->on_run();
        task->on_save(blob);
        tag = task->ID;
        
        planned.erase(planned.begin() + selected);
        return true;
    }
private:
    void add_task(Task*t){ tasks.push_back(t); t->ID = tasks.size()-1;}
    void submit(Task*t){ planned.push_back(t); }
private:
    vector<Task*> tasks;
    vector<Task*> planned;
    unsigned num_submitted;
};

Task::Task(TaskEngine&te):taskeng(te),submitted(false)
{
    te.add_task(this);
}

bool Task::submit(){
    if(submitted) return false;
    submitted = true;
    taskeng.num_submitted++;
    taskeng.submit(this);
    return true;
}

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
