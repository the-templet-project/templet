#include <string>
#include <list>
#include <map>

using namespace std;

struct TEMPLET_APP_INTERFACE{

public: // service interface    

typedef string   NAME;
typedef string   CID;
typedef string   PERMISSION; // permission info
typedef string   DATA;       // abstract binary data
typedef unsigned ORDINAL;
typedef unsigned TAG;
typedef bool     EXTERN;
typedef bool     ANSWER;

#define ASSERT(expr) if(!(expr))return false;

bool has_write_event(PERMISSION);
bool has_read_events(PERMISSION);
bool has_reply_on_query(PERMISSION);
bool has_client_actions(PERMISSION);

CID cid_from_data(DATA&);

struct TOKEN{
    NAME name;             // who is the token owner?
    PERMISSION permission; // what permissions does it have?
}; 

struct EVENT{
    EVENT(ORDINAL _ord,TAG _tag,EXTERN _ext, ANSWER _ans,NAME _name,DATA _data){
        ord=_ord; tag=_tag; ext=_ext; answer=_ans; name=_name; data=_data;
    }
    ORDINAL ord;
    TAG     tag;
    EXTERN  ext;  // ext==true used for client queries 
    ANSWER  answer; // if answer==true when 'tag' is the ordinal of the event, that is query  
    NAME    name;
    DATA    data; // if ext==true, data may include query GUID
};

/*-1-*/
bool write_event(TOKEN tkn,TAG tag,DATA data,ORDINAL& ord){  
    ASSERT(has_write_event(tkn.permission));
    
    EVENT ev(events.size(),tag,/*extern*/false,/*answer*/false,tkn.name,data);      
    ord = events.size();
    events.push_back(ev);
    return true;
}
    
/*-2-*/              
bool read_events(TOKEN tkn,ORDINAL ord,list<EVENT>& evs){
    ASSERT(has_read_events(tkn.permission));
    
    evs.clear(); 
    for(EVENT ev:events) if(ev.ord>=ord) evs.push_back(ev);
    return true;
}

/*-3-*/
bool write_query(TOKEN tkn,TAG tag,DATA data,ORDINAL& ord){
    ASSERT(has_client_actions(tkn.permission));
    
    EVENT ev(events.size(),tag,/*extern*/true,/*answer*/false,tkn.name,data);
    ord = events.size();
    events.push_back(ev);
    
    return true;
}

/*-4-*/
bool reply_on_query(TOKEN tkn,ORDINAL ord,DATA data){
    ASSERT(has_reply_on_query(tkn.permission));

    if(ord>=events.size()) return false;
    
    EVENT ev(events.size(),ord,/*extern*/false,/*answer*/true,tkn.name,data);
    events.push_back(ev);
 
    return true;
}    

/*-5-*/            
bool read_answer(TOKEN tkn,ORDINAL ord,DATA& data){
    ASSERT(has_client_actions(tkn.permission));
   
    bool allowed = false;
    
    for(EVENT ev:events){
        if(ev.ord==ord){ 
            if(ev.name==tkn.name) allowed = true;
            else return false;
        }
        if(ev.answer && ev.tag==ord && allowed){
            data = ev.data;
            return true;
        }
        if(ev.answer==ord && !allowed) return false;
    }
    return false;
}

/*-6-*/            
bool upload(TOKEN tkn,DATA data,CID& cid){
    ASSERT(has_client_actions(tkn.permission));
    cid = cid_from_data(data);
    user_local_bases[tkn.name][cid]=data;
    return true;
}

/*-7-*/            
bool download(TOKEN tkn,NAME cid,DATA& data){
    ASSERT(has_client_actions(tkn.permission));
    
    auto& base = user_local_bases[tkn.name];
    
    if(base.find(cid)!=base.end()){
        data = base[cid];
        return true;
    }
    else{
        for(auto& base:user_local_bases){
            if(base.second.find(cid)!=base.second.end()){
                data = base.second[cid];
                user_local_bases[tkn.name][cid]=data;
                return true;
            }
        }
    }
    return false;
}

/*-8-*/            
bool pin_cid(TOKEN tkn,NAME cid){
    ASSERT(has_client_actions(tkn.permission));
    DATA data;
    return download(tkn,cid,data);
}

/*-9-*/            
bool del_cid(TOKEN tkn,NAME cid){
    ASSERT(has_client_actions(tkn.permission));
    return user_local_bases[tkn.name].erase(cid)!=0;
}

/*-10-*/            
bool clear(TOKEN tkn){
    ASSERT(has_client_actions(tkn.permission));
    user_local_bases[tkn.name].clear();
    return true;
}

private: // service state
    
list<EVENT>  events;
map<NAME,map<CID,DATA>> user_local_bases;

};