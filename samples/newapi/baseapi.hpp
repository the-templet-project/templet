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
#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>

using namespace std;

typedef string BLOB;

class EventLog{
public:
     void write(unsigned tag,unsigned pid,const BLOB&blob){
        unique_lock lk(mut);
        EVENT ev{tag,pid,blob};
        events.push_back(ev);
        cv.notify_all();
    }
    bool read(unsigned ord,unsigned&tag,unsigned&pid,BLOB&blob){
        unique_lock lk(mut);
        if(ord<events.size()){
            tag = events[ord].tag; pid = events[ord].PID; blob = events[ord].blob;  
            return true;
        }
        return false;
    }
    void await_ord(unsigned ord){
        unique_lock lk(mut);
        while(!(ord<events.size())){cv.wait(lk);}
    }
private:
    struct EVENT{ unsigned tag; unsigned PID; BLOB blob; };
    vector<EVENT> events;
    unsigned next_add_event;
    mutex mut;
    condition_variable cv;
};

class InterfaceWorker{
public:
    InterfaceWorker(unsigned pid, EventLog&_log):log(_log),PID(pid),tag(0){}
public:
    void query(const BLOB&in,BLOB&out){
        unique_lock lk(mut);
        unsigned query_tag=tag++;
        log.write(query_tag,PID,in);
        map<unsigned,BLOB>::iterator it;
        while((it=answers.find(query_tag))==answers.end()){cv.wait(lk);}
        out = it->second;
        answers.erase(it);
    }
    void notify(unsigned tag,const BLOB&blob){ log.write(tag,PID,blob); }
public:
    void answer(unsigned tag,const BLOB&blob){
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
