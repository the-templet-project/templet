#include <cstdlib>
#include <iostream>
#include <vector>
#include <list>
//#include <concepts>
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

	void print() {
		for (int i = 0; i<log.size(); i++) {
			cout << "tag  = " << log[i].first << endl;
			cout << "data = \"" << log[i].second << "\"" << endl;
			cout << "======" << endl;
		}
	}
	
	int  add_event(int tag, string& data){ // returns num
	    omp_set_lock(&lock);
	    
	    for(int i=0; i < log.size(); i++)
	        if(log[i].first == tag) {omp_unset_lock(&lock);return i;}
	    
	    log.push_back(pair<int, string>(tag,data));
	    int num = static_cast<int>(log.size())-1;
	    
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
	bool is_ready() { return ready; }
	bool is_active() { return active; }
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
    	    
    	    int num_of_active_task = static_cast<int>(active_task_arr.size());
    	    if(num_of_active_task==0) return;
    	    
    	    int num_of_task_to_exec = rand() % num_of_active_task;
    	    map<int,task*>::const_iterator task_to_exec=active_task_arr.begin();
    	    for(int i=0; i!=num_of_task_to_exec; i++,task_to_exec++);
    	    
    	    task* t = task_to_exec->second;
    	    tag = task_to_exec->first;
    	    t->on_exec();
    	    
    	    ostringstream outs;
    	    t->on_save(outs);
			data = outs.str();
    	    
    	    my_task_num = an_event_log.add_event(tag,data);
	    }
	}

private:
	int current_num;
	int current_tag;
	int my_task_num;
	map<int,task*> active_task_arr;
};

//---------explicit----------//

const int N = 5;
int A[N][N], B[N][N];

class mult_task: public task{

protected:
    void on_exec() override{
       int i = cur_i;
		for(int j=0; j<N; j++){
		    c[j] = 0;
		    for(int k=0; k<N; k++) c[j] += A[i][k]*B[k][j];
		} 
    }

    void on_save(ostream& out) override{ 
        for(int j=0; j<N; j++) out << c[j] << " ";
    }
    
    void on_load(istream& in) override{
        for(int j=0; j<N; j++) in >> c[j];
    }

public:
    int cur_i;
	int c[N];
};

void check_result(int[N][N]);

void init_and_run()
{   
	int C[N][N];

	engine eng;
    mult_task task_arr[N];
    
	srand(0);

    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++){
            A[i][j] = rand();
            B[i][j] = rand();
            C[i][j] = 0;
        }
    
    for(int i = 0; i < N; i++){
        task_arr[i].cur_i = i;
        eng.submit(task_arr[i]);
    }

	eng.wait_all();

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			C[i][j] = task_arr[i].c[j];

	check_result(C);
}

//--------------------------------------------------
template<typename T>
//	requires(derived_from<T,task>)
class taskbag{

static void on_ready_handler(task*t, void*cntxt) {
		taskbag* _cntxt = static_cast<taskbag*>(cntxt);
		T*       _task = static_cast<T*>(t);

		_cntxt->on_put(*_task);
		_cntxt->idle_task_arr.push_back(_task);
		_cntxt->try_submit();
	}

public:
	taskbag(int num_workers) {
	    task_arr.resize(num_workers);
		for (int i = 0; i < num_workers; i++) {
			task_arr[i].set_on_ready_func(on_ready_handler,this);
			idle_task_arr.push_back(&task_arr[i]);
		}
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

//----------implicit---------//

const int NW = 2;

class mult_taskbag: public taskbag<mult_task>{

public:
    mult_taskbag(): taskbag(NW) {}
    
    void prep_and_run(){
        for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++){
            A[i][j] = rand();
            B[i][j] = rand();
            C[i][j] = 0;
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

	void on_put(mult_task&t) override {
		for (int j = 0; j < N; j++)	C[t.cur_i][j] = t.c[j];
	}

private:
    int cur_i;

public:
	int C[N][N];
};

//---------------------------------------------------

void check_result(int C1[N][N])
{
	int C2[N][N];

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			C2[i][j] = 0;
			for (int k = 0; k < N; k++) C2[i][j] += A[i][k] * B[k][j];
			//cout << C2[i][j] << " ";
			if (C1[i][j] != C2[i][j]) { cout << "test failed!!! "; return; }
		}
		//cout << endl;
	}
	cout << "test passed!!! ";
}

//---------------------------------------------------

int main()
{
	cout << "=== test for the explicit use of the task engine ===" << endl << endl;

	for(int i=0;i<10;i++)
	{
		an_event_log.reset();

#pragma omp parallel
		{
			init_and_run();
		}
	}

	cout << endl << endl << "=== test for the implicit use of the task engine ===" << endl << endl;

	for (int i = 0; i < 10; i++)
	{
		an_event_log.reset();

#pragma omp parallel   
		{
			mult_taskbag tb;
			tb.prep_and_run();
#pragma omp critical
		{
			//cout << endl << endl;
			//tb.print_result();
			//cout << endl << endl;
			check_result(tb.C);
		}
		}

		//cout << endl << endl;
		//an_event_log.print();

	}
	return EXIT_SUCCESS;
}
