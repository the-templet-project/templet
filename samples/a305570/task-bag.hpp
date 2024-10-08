#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

/////////////////////////////////////////////////////////////////////

class bag{

protected:
    class task{
    friend class bag;
        void on_run(){
            /*-1-*/
        }
            /*-2-*/
    }; 
    
public:
    bag(int _num_workers){  
        num_workers = _num_workers; 
            /*-3-*/          
    }
    ~bag(){  
            /*-4-*/          
    }
    bool on_has_task(){ 
            /*-5-*/
        return false;
    }
    void on_get(task&){
            /*-6-*/
    }
    void on_put(task&){
            /*-7-*/
    }

public:
            /*-8-*/

public:
    void run(){
        srand((unsigned)time(0));
        cur_num_workers = 0;

        for(;;){
            while(on_has_task() && (cur_num_workers < num_workers)){
                task* tsk = new task;
                on_get(*tsk);
                planned_task_arr.push_back(tsk);
                cur_num_workers++;
            }
            
            unsigned size; 
            if((size = planned_task_arr.size())==0) break;
            
            unsigned selected = rand() % size;
            task* selected_task = planned_task_arr[selected];
            planned_task_arr.erase(planned_task_arr.begin()+selected);

            selected_task->on_run();
            on_put(*selected_task);
            delete selected_task;
        }        
    }

private:
    int cur_num_workers;
    int num_workers;
    vector<task*> planned_task_arr;
};

/////////////////////////////////////////////////////////////////////

class seq_test_bag{

protected:
    class task{
    friend class seq_test_bag;
        void on_run(){
            /*-1-*/
        }
            /*-2-*/
    }; 
    
public:
    seq_test_bag(){  
            /*-3-*/          
    }
    ~seq_test_bag(){  
            /*-4-*/          
    }
    bool on_has_task(){ 
            /*-5-*/
        return false;
    }
    void on_get(task&){
            /*-6-*/
    }
    void on_put(task&){
            /*-7-*/
    }

public:
            /*-8-*/

public:
    void run(){
        task tsk;
        while(on_has_task()){
            on_get(tsk);
            tsk.on_run();
            on_put(tsk);
        }
    }
};

/////////////////////////////////////////////////////////////////////

class seq_test{
public:
    void run(){
            /* seq code */
    };
};