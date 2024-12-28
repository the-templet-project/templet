#include <string>
#include <list>
#include <map>
#include <istream>
#include <ostream>

using namespace std;

struct TEMPLET_APP_INTERFACE{

public:

typedef unsigned PERMISSIONS;
typedef unsigned ORDINAL;
typedef string   NAME;
typedef unsigned TAG;
typedef string   DATA;

enum TYPE     {INTERNAL, EXTERNAL};

const unsigned OBSERV_PERMISSION = 0x1;
const unsigned WORKER_PERMISSION = 0x2;
const unsigned CLIENT_PERMISSION = 0x4;
const unsigned SERVER_PERMISSION = 0x8;

const unsigned ATTEMPTS_COUNT =  30;

struct TOKEN{
    NAME name;            
    PERMISSIONS permissions; 
}; 

struct EVENT{
    EVENT(ORDINAL _ord, NAME _name, TYPE _type, TAG _tag, DATA _data){
        ord=_ord; name=_name; type=_type; tag=_tag; data=_data;
    }
    ORDINAL ord;
    NAME    name;
    TYPE    type;
    TAG     tag;
    DATA    data;
};

/*-1-*/
bool write_event(TOKEN tkn,TAG tag,DATA data){
    if(!(tkn.permissions & WORKER_PERMISSION))return false; 
    try{
        EVENT ev(events.size(),tkn.name,TYPE::INTERNAL,tag,data);      
        events.push_back(ev);
    }
    catch(...){ return false;}
    return true;
}
    
/*-2-*/              
bool read_event(TOKEN tkn,ORDINAL ord,EVENT& evs){
    if(!(tkn.permissions & OBSERV_PERMISSION))return false; 
    try{
        for(EVENT ev:events) if(ev.ord==ord){ evs=ev; return true;}
    }
    catch(...){ return false;}
    return false;
}

/*-3-*/
bool write_query(TOKEN tkn,TAG tag,DATA in_data,DATA& out_data){
    if(!(tkn.permissions & CLIENT_PERMISSION))return false;
    try{ 
        ORDINAL query_ord = events.size();
        EVENT ev(query_ord,tkn.name,TYPE::EXTERNAL,tag,in_data);
        events.push_back(ev);
        
        for(int i=0;i<ATTEMPTS_COUNT;i++){
            //delay
            map<ORDINAL,DATA>::iterator it = answers.find(query_ord);
            if(it!=answers.end()){
                out_data = it->second;
                answers.erase(it);
                return true;
            }
        }
    }
    catch(...){return false;}
    return false;
}

/*-4-*/
bool reply_on_query(TOKEN tkn,ORDINAL query_ord,DATA data){
    if(!(tkn.permissions & SERVER_PERMISSION))return false;
    try{
        answers[query_ord] = data;
        for(int i=0;i<ATTEMPTS_COUNT;i++){
            //delay;
            map<ORDINAL,DATA>::iterator it = answers.find(query_ord);
            if(it==answers.end()) return true;
        }
    }
    catch(...){return false;}
    answers.erase(query_ord);
    return false;
}    

private:
    
list<EVENT>       events;
map<ORDINAL,DATA> answers;
};


class AppModel{

public:
    AppModel(){current_event=0; next_add_event=0;}
    
    void run(){
        for(;;){
            for(;;){
                try{ 
                    EVENT ev = events.at(current_event);
                    on_read(current_event,ev.tag,ev.ext,ev.size,ev.buf);
                    current_event++;
                }
                catch(const std::out_of_range& ex){break;}
            }    
            unsigned tag; unsigned size; void *buf;
            while(on_write(tag,size,buf)){
                EVENT ev{tag,false,size,buf};
                events[next_add_event++] = ev;
            };
        }
    }

public:
    void notify(unsigned tag,unsigned size,void*buf){
        EVENT ev{tag,true,size,buf};
        events[next_add_event++] = ev;
    }

    bool query(unsigned tag,unsigned size_in,void*buf_in,unsigned& size_out,void*&buf_out){

        EVENT ev{tag,true,size_in,buf_in};
        events[next_add_event] = ev;
        unsigned my_query_event = next_add_event++; 

        for(int i=0;i<999999;i++){
            try{
                pair<unsigned,void*> answer = answers.at(my_query_event);
                size_out = answer.first;
                buf_out = answer.second;
                return true;
            }
            catch(const std::out_of_range& ex){}
        }
        return false;
    }

protected:
    void answer(unsigned ord,unsigned size,void*buf){
        answers[ord]=pair(size,buf);
    }

    virtual void on_read(unsigned ord,unsigned tag,bool ext,unsigned& size,void*&buf)=0;
    virtual bool on_write(unsigned& tag,unsigned& size,void*&buf)=0;

private:
    struct EVENT{ unsigned tag; bool ext; unsigned size; void*buf; };
    map<unsigned,EVENT> events;
    unsigned current_event;
    unsigned next_add_event;
    map<unsigned,pair<unsigned,void*>> answers;
};

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
