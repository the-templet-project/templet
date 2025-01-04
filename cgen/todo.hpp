#include <map>
#include <mutex>
#include <condition_variable>

using namespace std;

struct BLOB{unsigned size;void*buf;};

class EventLog{
public:
    virtual bool try_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob)=0;
    virtual void wait_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob)=0;
    virtual unsigned write(unsigned tag,unsigned&pid,BLOB blob)=0;
};

class CirleBufEventLog: public EventLog{
public:
    bool try_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob) override{
        return false;
    }
    void wait_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob) override{
        
    }
    unsigned write(unsigned tag,unsigned&pid,BLOB blob) override{
        return 0;
    }
};

class SimpleEventLog: public EventLog{
public:
    bool try_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob) override{
        unique_lock lk(mut);
        map<unsigned,EVENT>::iterator it; 
        if((it=events.find(ord))!=events.end()){
            tag = it->second.tag; pid = it->second.PID; blob = it->second.blob;
            return true;
        }
        return false;
    }
    void wait_read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob) override{
        unique_lock lk(mut);
        map<unsigned,EVENT>::iterator it; 
        while((it=events.find(ord))==events.end()){cv.wait(lk);}
        tag = it->second.tag; pid = it->second.PID; blob = it->second.blob;  
    }
    unsigned write(unsigned tag,unsigned&pid,BLOB blob) override{
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

class AppProcess{
protected:
    AppProcess(unsigned pid, EventLog&_log):log(_log),PID(pid),current_event(0){}
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
    virtual bool on_read(unsigned ord,unsigned tag,unsigned pid,BLOB&blob)=0;
    virtual bool on_write(unsigned&tag,BLOB&blob)=0;
private:
    EventLog& log;
    unsigned current_event;
    unsigned PID;
};

class ComProcess{
protected:
    ComProcess(unsigned pid, EventLog&_log):log(_log),PID(pid){}
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
