#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

class bag_seq{

protected:
    class task{  
        friend class bag_seq; 
        virtual void on_run()=0;
    }; 
    
    virtual task* create_task()=0; 
    
    virtual bool on_get(task&)=0;
    virtual void on_put(task&)=0;

public:
    bag_seq(int num_wokers){ }
    
    void run(){
        task* t=create_task();
        while(on_get(*t)){
            t->on_run();
            on_put(*t);
        }
    }
};

class bag_par{

protected:
    class task{
    public:
        virtual ~task()=0; 
        virtual void on_run()=0;
    }; 
    
    virtual task* create_task()=0;
    
    
    virtual bool on_get(task&)=0;
    virtual void on_put(task&)=0;

public:
    bag_par(int _num_workers){ num_workers = _num_workers; }

    void run(){
        srand((unsigned)time(0));
        cur_num_workers = 0;
        task* t=create_task();

        for(;;){
            while(on_get(*t) && (cur_num_workers < num_workers)){
                planned_task_arr.push_back(t);
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
