#include <stdlib.h>
#include <iostream>
#include <vector>
#include <concepts>

using namespace std;

//--------------------------------------------------------

class event_log{
public:
    void reset() {log.clear();}
	
	bool  add_event(int tag, string& data){ // returns num
	    for(int i=0; i < log.size(); i++)
	        if(log[i].first == tag) return false;
	    log.push_back(pair(tag,data));
	    return true; 
	}
	
	bool get_event(int num, string& data){
	    if(0<=num && num < log.size()){
	        data = log[num].second;
	        return true;
	    }
	    return false;
	}
	
private:
	vector<pair<int,string>> log;
};

//--------------------------------------------------------

class task{
	
public:
    bool is_ready();
    bool is_active();
protected:
    virtual void on_exec() {}
    virtual void on_ready(){}
    virtual void on_save(ostream&) {}
    virtual void on_load(istream&) {}
private:
	bool ready;
	bool active;
};

class engine{

public:
	void submit(task&) {}
	void wait_all() {}
};

//--------------------------------------------------
// explicit use of task engine

const int N = 10;
double A[N][N], B[N][N], C[N][N];

class mult_task: public task{

    void on_exec() override {
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
    
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++) cout << C[i][j] << " ";
        cout << endl;
    }
    cout << endl;
}

//--------------------------------------------------

template<typename T>
	requires(derived_from<T,task>)
class taskbag{

public:
	taskbag(int num_workers) {}
	void run(int NW) {}
protected:
	virtual bool on_get(T&){ return false;}
	virtual void on_put(T&){}
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
        taskbag::run(NW);
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
	
	virtual void on_put(mult_task&t) override {
		cout << "getting row " << t.cur_i << "computed\n";
	}
	
private:
    int cur_i;
}; 

//---------------------------------------------------

int main()
{
    cout << "hello!!!\n\n";
    ////////////////////////
    init_and_run();
    ////////////////////////
    mult_taskbag tb;
    
    tb.prep_and_run();
    tb. print_result();
    ////////////////////////
    return EXIT_SUCCESS;
}