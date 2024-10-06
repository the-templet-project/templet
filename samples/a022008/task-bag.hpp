#include <iostream>
#include <list>
#include <vector>

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
        friend class bag_par; 
        virtual void on_run()=0;
    }; 
    
    virtual task* create_task()=0; 
    
    virtual bool on_get(task&)=0;
    virtual void on_put(task&)=0;

    void init(){
        for(int i=0; i<num_workers; i++)
            ready_task_list.push_back(create_task());
    }

public:
    bag_par(int _num_workers){ num_workers = _num_workers; }

    void run(){
        init();
        task* t=create_task();
        while(on_get(*t)){
            t->on_run();
            on_put(*t);
        }
    }

private:
    int num_workers;
    vector<task*> planned_task_arr;
    list<task*>   ready_task_list;
};
