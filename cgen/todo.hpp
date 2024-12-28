#include <map>
#include <mutex>
#include <condition_variable>

using namespace std;

class EventLog{
public:
    bool try_read(unsigned ord,unsigned&tag,unsigned&pid,unsigned&size,void*&buf){
        unique_lock lk(mut);
        map<unsigned,EVENT>::iterator it; 
        if((it=events.find(ord))!=events.end()){
            tag = it->second.tag; pid = it->second.PID;
            size = it->second.size; buf = it->second.buf;
            return true;
        }
        return false;
    }
    void wait_read(unsigned ord,unsigned&tag,unsigned&pid,unsigned&size,void*&buf){
        unique_lock lk(mut);
        map<unsigned,EVENT>::iterator it; 
        while((it=events.find(ord))==events.end()){cv.wait(lk);}
        tag = it->second.tag; pid = it->second.PID;
        size = it->second.size; buf = it->second.buf;  
    }
    void write(unsigned tag,unsigned&pid,unsigned size,void*buf){
        unique_lock lk(mut);
        EVENT ev{tag,pid,size,buf};
        events[next_add_event++] = ev;
        cv.notify_all();
    }
private:
    struct EVENT{ unsigned tag; unsigned PID; unsigned size; void*buf; };
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
            unsigned tag; unsigned pid; unsigned size; void*buf;
            
            log.wait_read(current_event,tag,pid,size,buf);
            on_read(current_event,tag,pid,size,buf);
            current_event++;
            
            while(log.try_read(current_event,tag,pid,size,buf)){
                on_read(current_event,tag,pid,size,buf);
                current_event++;
            }

            if(on_write(tag,size,buf))log.write(tag,PID,size,buf);
        } 
    }
protected:
    virtual void on_read(unsigned ord,unsigned tag,unsigned pid,unsigned&size,void*&buf)=0;
    virtual bool on_write(unsigned&tag,unsigned&size,void*&buf)=0;
private:
    EventLog& log;
    unsigned current_event;
    unsigned PID;
};
