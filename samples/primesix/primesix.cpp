/*$TET$$header*/
#include "primeops.h"

#include <chrono>
#include <cmath>
#include <cassert>
#include <iostream>
#include <list>
#include <vector>
#include <algorithm>

#include <templet.hpp>
#include <basesim.hpp>

using namespace std;

const long long SEARCH_RANGE_LIMIT       = 100000000;
const int  NUMBER_OF_SEARCH_RANGE_CHANKS = 200;

const int  NUMBER_OF_TASK_SLOTS    = 10;
const int  NUMBER_OF_APP_INSTANCES = 10;

class request : public templet::message {
public:
	request(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) {}
    bool   input;  
    int    tag;                //in
    list<long long> sextuplets;//in
    int    ord;                //out
};

/*event_log=array_of(tag,list_of(sextuplets))*/
vector<pair<int,list<long long>>> event_log;
int NUM_OF_RUNNING_APP_INSTANCES;

/*$TET$*/

#pragma templet tsync(r?request)

struct tsync :public templet::actor {
	static void on_r_adapter(templet::actor*a, templet::message*m) {
		((tsync*)a)->on_r(*(request*)m);}

	tsync(templet::engine&e) :tsync() {
		tsync::engines(e);
	}

	tsync() :templet::actor(false)
	{
/*$TET$tsync$tsync*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$tsync$engines*/
/*$TET$*/
	}

	inline void on_r(request&m) {
/*$TET$tsync$r*/
        if(m.input) event_log.push_back(pair(m.tag,m.sextuplets));
        m.ord = event_log.size() - 1; 
        m.send();
/*$TET$*/
	}

	void r(request&m) { m.bind(this, &on_r_adapter); }

/*$TET$tsync$$footer*/
/*$TET$*/
};

#pragma templet !tcall(r!request,t:basesim)

struct tcall :public templet::actor {
	static void on_r_adapter(templet::actor*a, templet::message*m) {
		((tcall*)a)->on_r(*(request*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((tcall*)a)->on_t(*(templet::basesim_task*)t);}

	tcall(templet::engine&e,templet::basesim_engine&te_basesim) :tcall() {
		tcall::engines(e,te_basesim);
	}

	tcall() :templet::actor(true),
		r(this, &on_r_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$tcall$tcall*/
        task_selector.init();
        task_sorter.init();
        task_ord_to_start_update_from = 0;
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$tcall$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$tcall$start*/
        r.input = false;
        r.send();
/*$TET$*/
	}

	inline void on_r(request&m) {
/*$TET$tcall$r*/
        t.submit();
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$tcall$t*/
        for(int i=task_ord_to_start_update_from; i<=r.ord; i++){
            if(task_selector.task_ready(event_log[i].first))
                task_sorter.add_ready(event_log[i].first,event_log[i].second);
        }
        task_ord_to_start_update_from = r.ord+1;

        int task_tag;
        if(task_selector.select_task(task_tag)){
            // process task
            r.tag = task_tag; r.sextuplets.clear();

            r.input = true;
            t.delay(1.0);
        } 
        else{
            r.input = false;
            t.delay(0.0); 
        }
        
        if(task_sorter.completed()){
            if(--NUM_OF_RUNNING_APP_INSTANCES==0) stop();
        }
        else r.send();
/*$TET$*/
	}

	request r;
	templet::basesim_task t;

/*$TET$tcall$$footer*/
    int task_ord_to_start_update_from;

struct planned_task_selector{
    void init(){
        srand(0);
        planned.resize(min(NUMBER_OF_TASK_SLOTS,NUMBER_OF_SEARCH_RANGE_CHANKS));
        int i; for(i=0;i<planned.size();i++) planned[i]=i;
        last_planned = i-1;       
    }
    bool select_task(int&task_tag){
        if(planned.size()==0)return false;
        task_tag = planned[rand()%planned.size()];
        return true;
    }
    bool task_ready(int&task_tag){
        for(auto it=planned.begin();it!=planned.end();it++)
            if(*it==task_tag){
                if(last_planned == NUMBER_OF_SEARCH_RANGE_CHANKS-1){planned.erase(it); return true;}
                else{ *it=++last_planned; return true; }
            }
        return false;
    }
    vector<int> planned;
    int last_planned;
} task_selector;

struct ready_task_sorter{
    void init(){
        print_enabled = false;
        last_printed_task=-1;last_printed_sextuplet=0;
        ready.clear();
    } 
    bool add_ready(int tag,list<long long>& sextuplets){
        ready.push_back(pair(tag,sextuplets));
        ready.sort([](const auto& l, const auto& r){return l.first < r.first;});
        
        for(auto it = ready.begin(); it != ready.end();){
            if(it->first==last_printed_task+1){
                last_printed_task++;
                // print
                if(print_enabled){
                    cout<<"task tag #" << last_printed_task << endl;
                    for(auto n:it->second) cout<<"sextuplet #" << ++last_printed_sextuplet << " ("<< n <<")" << endl;
                }
                it = ready.erase(it);
            }
            else break;
        }
        return true;
    }
    bool completed(){return last_printed_task==NUMBER_OF_SEARCH_RANGE_CHANKS-1;}
    bool print_enabled;
    int last_printed_task;
    int last_printed_sextuplet;
    list<pair<int,list<long long>>> ready;
} task_sorter;

/*$TET$*/
};

/*$TET$$footer*/

void run_state_sync_app()
{
	templet::engine eng;
	templet::basesim_engine teng;

    event_log.clear();
    NUM_OF_RUNNING_APP_INSTANCES = NUMBER_OF_APP_INSTANCES;

	tsync log(eng);
	tcall app[NUMBER_OF_APP_INSTANCES];

	for (int i = 0; i < NUMBER_OF_APP_INSTANCES; i++) {
		app[i].engines(eng,teng);
		log.r(app[i].r);
	}

    app[0].task_sorter.print_enabled = true;
    
	eng.start();
	teng.run();

	if (!eng.stopped()) {cout << "Logical error" << std::endl; return;}

	cout << "Maximum number of tasks executed in parallel : " << teng.Pmax() << endl;
	cout << "Time of sequential execution of all tasks    : " << teng.T1() << endl;
	cout << "Time of parallel   execution of all tasks    : " << teng.Tp() << endl;
	cout << "Estimated speed-up                           : " << teng.T1() / teng.Tp() << std::endl;    
}

void run_sequential_test()
{
    list<long long> table;

    cout << endl << "run_sequential_test()" << endl;
        
    auto start = std::chrono::high_resolution_clock::now();
	sextuplets_search(table,SEARCH_RANGE_LIMIT);
	auto end = std::chrono::high_resolution_clock::now();
    
	std::chrono::duration<double> diff = end - start;	
    cout << "exec.time = " << diff.count() << "s" << endl;
}

void run_prime_funcs_test()
{
    list<long long> table;
    list<long long> found;

    cout << endl << "run_prime_funcs_test()" << endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    long long max_num_in_table = (long long)sqrt(SEARCH_RANGE_LIMIT);
    cout << "max_num_in_table = " << max_num_in_table << endl; 
    extend_prime_table(table, max_num_in_table);
    
    find_sextuplets_in_range(table,3,SEARCH_RANGE_LIMIT,found);
 
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end - start;	
    cout << "exec.time = " << diff.count() << "s" << endl;
}

int main()
{
    //run_sequential_test();
    //run_prime_funcs_test();
    run_state_sync_app();

    return EXIT_SUCCESS;
}
/*$TET$*/
