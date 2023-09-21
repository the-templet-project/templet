#include <cstdlib>
#include <iostream>
#include <vector>
#include <list>
#include <concepts>
#include <functional>
#include <climits>
#include <map>
#include <cassert>
#include <sstream>

#include <omp.h>

using namespace std;

//--------------------------------------------------------

class event_log{
public:
    event_log(){ omp_init_lock(&lock); }
    ~event_log(){ omp_destroy_lock(&lock); }
    
    void reset() {log.clear();}
	
	int  add_event(int tag, string& data){ // returns num
	    omp_set_lock(&lock);
	    
	    for(int i=0; i < log.size(); i++)
	        if(log[i].first == tag) {omp_unset_lock(&lock);return i;}
	    
	    log.push_back(pair(tag,data));
	    int num = log.size()-1;
	    
	    omp_unset_lock(&lock);
	    return num; 
	}
	
	int get_event(int num, string& data){ // returns tag
	    omp_set_lock(&lock);
	    
	    if(0<=num && num < log.size()){
	        int tag = log[num].first;
	        data = log[num].second;
	        omp_unset_lock(&lock);
	        return tag;
	    }
	    
	    omp_unset_lock(&lock);
	    return INT_MAX;
	}
	
private:
	vector<pair<int,string>> log;
    omp_lock_t lock;
    
} an_event_log;

//--------------------------------------------------------

class task{
friend class engine;
	
public:
    bool is_ready();
    bool is_active();
    void set_on_ready_func(function<void(task*,void*)>f,void*cnxt){
         func_on_ready=f; on_ready_cnxt=cnxt; 
    }

private:
    function<void(task*,void*)> func_on_ready=[](task*,void*){};
    void* on_ready_cnxt;
    
protected:
    virtual void on_exec() {}
    virtual void on_ready(){ func_on_ready(this,on_ready_cnxt); }
    virtual void on_save(ostream&) {}
    virtual void on_load(istream&) {}
    
private:
	bool ready =false;
	bool active=false;
};

class engine{

public:
    engine(){
        an_event_log.reset(); 
        current_num = 0;
        current_tag = 0;
        my_task_num = INT_MAX;
    }
    
	void submit(task&t) {
	    assert(t.active==false);
	    active_task_arr[current_tag++]=&t;
	    t.active = true;
        t.ready = false;
	}
	
	void wait_all() {
	    int tag; string data;
	    
	    for(;;){
    	    while((tag = an_event_log.get_event(current_num,data))!=INT_MAX){
    	        
    	        task* t = active_task_arr[tag];
    	        active_task_arr.erase(tag);
    	        
    	        if(current_num!=my_task_num){
    	            istringstream ins(data);
    	            t->on_load(ins);
    	        }
    	        
    	        t->active = false;
    	        t->ready = true;
    	        t->on_ready();
    	    
    	        current_num++;
    	    }
    	    
    	    int num_of_active_task = active_task_arr.size();
    	    if(num_of_active_task==0) return;
    	    
    	    int num_of_task_to_exec = rand() % num_of_active_task;
    	    map<int,task*>::const_iterator task_to_exec=active_task_arr.begin();
    	    for(int i=0; i!=num_of_task_to_exec; i++,task_to_exec++);
    	    
    	    task* t = task_to_exec->second;
    	    tag = task_to_exec->first;
    	    t->on_exec();
    	    
    	    data.clear();
    	    ostringstream outs(data);
    	    t->on_save(outs);
    	    
    	    my_task_num = an_event_log.add_event(tag,data);
	    }
	}

private:
	int current_num;
	int current_tag;
	int my_task_num;
	map<int,task*> active_task_arr;
};

//--------------------------------------------------
// explicit use of task engine

const int N = 10;
double A[N][N], B[N][N], C[N][N];

class mult_task: public task{

    void on_exec() override{
       int i = cur_i;
		for(int j=0; j<N; j++){
		    C[i][i] = 0.0;
		    for(int k=0; k<N; k++) C[i][j] += A[i][k]*B[k][j];
		} 
    }

    void on_save(ostream& out) override{
        for(int j=0; j<N; j++) out << C[cur_i][j];
    }
    
    void on_load(istream& in) override{
        for(int j=0; j<N; j++) in >> C[cur_i][j];
    }

public:
    int cur_i;
};

void init_and_run(){
    engine eng;
    
    mult_task task_arr[N];
    
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++){
            A[i][j] = rand()/10;
            B[i][j] = rand()/10;
            C[i][j] = 0.0;
        }
    
    for(int i = 0; i < N; i++){
        task_arr[i].cur_i = i;
        eng.submit(task_arr[i]);
    }
    
    eng.wait_all();
 
#pragma omp master    
{        
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++) cout << C[i][j] << " ";
        cout << endl;
    }
    cout << endl;
}
}

//--------------------------------------------------

template<typename T>
	requires(derived_from<T,task>)
class taskbag{

friend void on_ready_handler(task*t,void*cntxt){
    taskbag* _cntxt = static_cast<taskbag*>(cntxt);
    T*       _task  = static_cast<T*>(t);
    
    _cntxt->on_put(*_task);
    _cntxt->idle_task_arr.push_back(_task);
    _cntxt->try_submit();
}

public:
	taskbag(int num_workers) {
	    task_arr.resize(num_workers);
	    for(int i=0; i<num_workers; i++)
	        idle_task_arr.push_back(&task_arr[i]); 
	}
	
	void run() { try_submit(); eng.wait_all(); }

protected:
	virtual bool on_get(T&){ return false; }
	virtual void on_put(T&){}

private:
    void try_submit(){
         T* task;
         while(!idle_task_arr.empty() && 
                on_get(*(task=idle_task_arr.back()))){
             eng.submit(*task);
             idle_task_arr.pop_back();
         }  
    }
    
    engine eng;
    vector<T> task_arr;
    list<T*>  idle_task_arr;
};

//--------------------------------------------------
// implicit use of the task engine through an algorithmic skeleton

const int NW = 2;

class mult_taskbag: public taskbag<mult_task>{

public:
    mult_taskbag(): taskbag(NW) {}
    
    void prep_and_run(){
        for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++){
            A[i][j] = rand()/10;
            B[i][j] = rand()/10;
            C[i][j] = 0.0;
        }
        cur_i = 0;
        taskbag::run();
    }
    
    void print_result(){
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++) cout << C[i][j] << " ";
            cout << endl;
        }
        cout << endl;
    }
    
protected:    
	bool on_get(mult_task&t) override {
	    if(cur_i < N){ t.cur_i = cur_i++; return true; }
		return false;
	}

private:
    int cur_i;
}; 

//---------------------------------------------------

int main()
{
    /*
#pragma omp parallel
{
    init_and_run();
}   
#pragma omp parallel    
{
    mult_taskbag tb;
    tb.prep_and_run();
#pragma omp master    
    tb.print_result();
}*/
    return EXIT_SUCCESS;
}
