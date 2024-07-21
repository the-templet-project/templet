#include <string>
#include <list>
#include <map>

using namespace std;

struct TEMPLET_APP_INTERFACE{

public:

typedef unsigned PERMISSIONS;
typedef unsigned ORDINAL;
typedef string   NAME;
typedef unsigned TAG;
typedef string   DATA;

enum TYPE     {INTERNAL, QUERY, ANSWER};

const unsigned OBSERV_PERMISSION = 0x1;
const unsigned WORKER_PERMISSION = 0x2;
const unsigned CLIENT_PERMISSION = 0x4;
const unsigned SERVER_PERMISSION = 0x8;

const unsigned DELAY_COUNT =  30; // 30 seconds
const unsigned FILE_LIMIT  =  100;

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
bool reply_on_query(TOKEN tkn,ORDINAL query_ord,DATA data){
    if(!(tkn.permissions & SERVER_PERMISSION))return false;
    try{
        EVENT ev(events.size(),tkn.name,TYPE::ANSWER,query_ord,data);
        events.push_back(ev);
    }
    catch(...){return false;}
    return true;
}    

/*-4-*/
bool write_query(TOKEN tkn,TAG tag,DATA in_data,DATA& out_data){
    if(!(tkn.permissions & CLIENT_PERMISSION))return false;
    try{ 
        ORDINAL query_ord = events.size();
        EVENT ev(query_ord,tkn.name,TYPE::QUERY,tag,in_data);
        events.push_back(ev);
        for(int i=0;i<DELAY_COUNT;i++)
        for(EVENT ev:events){
            if(ev.type==TYPE::ANSWER && ev.tag==query_ord){
                out_data = ev.data; return true;
            }
        }
    }
    catch(...){return false;}
    return false;
}

/*-5-*/            
bool upload_file(TOKEN tkn,NAME local_file_name,DATA data,NAME& global_file_name){
    if(!tkn.permissions) return false;
    try{
        try{limits.at(tkn.name);} catch(...){limits[tkn.name]=0;}            
        if(limits[tkn.name]==FILE_LIMIT) return false;
        
        NAME global_file_name = tkn.name + "/" + local_file_name;
        files[global_file_name]=data;
        limits[tkn.name]++;
    }
    catch(...){return false;}
    return true;
}

/*-6-*/            
bool download_file(TOKEN tkn,NAME global_file_name,DATA& data){
    if(!tkn.permissions) return false;
    try{ data=files[global_file_name]; }
    catch(...){return false;}
    return true;
}

/*-7-*/            
bool delete_file(TOKEN tkn,NAME global_file_name){
    if(!tkn.permissions) return false;
    try{
        string::size_type npos = global_file_name.find_first_of('/');
        string name = global_file_name.substr(0,npos);
        if(name!=tkn.name) return false;
        if(!files.erase(global_file_name))return false;
        limits[tkn.name]--;
    }
    catch(...){return false;}
    return true;
}

private:
    
list<EVENT>        events;
map<NAME,DATA>     files;
map<NAME,unsigned> limits;

};
