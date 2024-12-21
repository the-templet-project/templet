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
    void run(unsigned num_proc);
    void run();
public:
    void notify(unsigned tag,istream&);
    bool query(unsigned tag,istream&,ostream&);
protected:
    void reply(unsigned tag,istream&);

    virtual void on_read(unsigned ord,unsigned tag,bool ext,istream&)=0;
    virtual bool on_write(unsigned tag,ostream&)=0;
};

