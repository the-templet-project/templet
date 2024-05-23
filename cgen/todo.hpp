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

enum TYPE     {INTERNAL, QUERY, ANSWER, FILE};
enum FILE_TAG {UPLOAD, DOWNLOAD, DELETE};

const unsigned WRITE_EVENT_PERMISSION    = 0x1;
const unsigned READ_EVENT_PERMISSION     = 0x2;
const unsigned WRITE_QUERY_PERMISSION    = 0x4;
const unsigned REPLY_ON_QUERY_PERMISSION = 0x8;
const unsigned READ_ANSWER_PERMISSION    = 0x10;
const unsigned UPLOAD_FILE_PERMISSION    = 0x20;    
const unsigned DOWNLOAD_FILE_PERMISSION  = 0x40;
const unsigned DELETE_FILE_PERMISSION    = 0x80;

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
bool write_event(TOKEN tkn,TAG tag,DATA data,ORDINAL& ord){
    if(!(tkn.permissions & WRITE_EVENT_PERMISSION))return false; 
    try{
        EVENT ev(events.size(),tkn.name,TYPE::INTERNAL,tag,data);      
        ord = events.size(); events.push_back(ev);
    }
    catch(...){ return false;}
    return true;
}
    
/*-2-*/              
bool read_event(TOKEN tkn,ORDINAL ord,EVENT& evs){
    if(!(tkn.permissions & READ_EVENT_PERMISSION))return false; 
    try{
        for(EVENT ev:events) if(ev.ord==ord){ evs=ev; return true;}
    }
    catch(...){ return false;}
    return false;
}

/*-3-*/
bool write_query(TOKEN tkn,TAG tag,DATA data,ORDINAL& ord){
    if(!(tkn.permissions & WRITE_QUERY_PERMISSION))return false;
    try{
        EVENT ev(events.size(),tkn.name,TYPE::QUERY,tag,data);   
        ord = events.size();  events.push_back(ev);
    }
    catch(...){return false;}
    return true;
}

/*-4-*/
bool reply_on_query(TOKEN tkn,ORDINAL query_ord,DATA data){
    if(!(tkn.permissions & REPLY_ON_QUERY_PERMISSION))return false;
    try{
        EVENT ev(events.size(),tkn.name,TYPE::ANSWER,query_ord,data);
        events.push_back(ev);
    }
    catch(...){return false;}
    return true;
}    

/*-5-*/            
bool read_answer(TOKEN tkn,ORDINAL query_ord,DATA& data){
    if(!(tkn.permissions & READ_ANSWER_PERMISSION))return false;
    try{
        bool allowed = false;
        for(EVENT ev:events){
            if(ev.type==TYPE::QUERY && ev.ord==query_ord){ 
                if(ev.name==tkn.name) allowed = true;
                else return false;
            }
            if(ev.type==TYPE::ANSWER && ev.tag==query_ord){
                if(allowed){ data = ev.data; return true;}
                else return false;
            }
        }
    }
    catch(...){return false;}
    return false;
}

/*-6-*/            
bool upload_file(TOKEN tkn,NAME local_file_name,DATA data,NAME& global_file_name){
    if(!(tkn.permissions & UPLOAD_FILE_PERMISSION))return false;
    try{
        NAME global_file_name = local_file_name + "." + tkn.name;
        files[global_file_name]=data;
        EVENT ev(events.size(),tkn.name,TYPE::FILE,FILE_TAG::UPLOAD,global_file_name);      
        events.push_back(ev);
    }
    catch(...){return false;}
    return true;
}

/*-7-*/            
bool download_file(TOKEN tkn,NAME global_file_name,DATA& data){
    if(!(tkn.permissions & DOWNLOAD_FILE_PERMISSION))return false;
    try{
        data=files[global_file_name];
        EVENT ev(events.size(),tkn.name,TYPE::FILE,FILE_TAG::DOWNLOAD,global_file_name);      
        events.push_back(ev);
    }
    catch(...){return false;}
    return true;
}

/*-8-*/            
bool delete_file(TOKEN tkn,NAME global_file_name){
    if(!(tkn.permissions & DELETE_FILE_PERMISSION))return false;
    try{
        if(!files.erase(global_file_name))return false;
        EVENT ev(events.size(),tkn.name,TYPE::FILE,FILE_TAG::DELETE,global_file_name);      
        events.push_back(ev);
    }
    catch(...){return false;}
    return true;
}

private:
    
list<EVENT>    events;
map<NAME,DATA> files;

};
