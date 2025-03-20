#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

struct task{
    /*-1-*/
};

struct task_pool{
    /*-2-*/
};

bool on_get(task&,task_pool&)
{
    /*-3-*/
    return false;
}

void on_run(task&)
{ 
    /*-4-*/
}

void on_put(task&,task_pool&)
{
    /*-5-*/
}

void task_pool_run(task_pool& tpool){
    task planned_task;
    vector<task> planned_task_arr;
   
    while(on_get(planned_task,tpool)) planned_task_arr.push_back(planned_task);

    unsigned size;
    for(srand((unsigned)time(0));!planned_task_arr.empty();){

        unsigned selected = rand() % planned_task_arr.size();
        task selected_task = planned_task_arr[selected];
        planned_task_arr.erase(planned_task_arr.begin()+selected);

        on_run(selected_task);
        on_put(selected_task,tpool);
    }        
}
