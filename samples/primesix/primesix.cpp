/*$TET$$header*/
#include "primeops.h"

#include <chrono>
#include <cmath>
#include <cassert>
#include <iostream>
#include <list>

#include <templet.hpp>
#include <basesim.hpp>

using namespace std;

const long long SEARCH_RANGE_LIMIT       = 100000000;
const int  NUMBER_OF_SEARCH_RANGE_CHANKS = 100;

const int  NUMBER_OF_TASK_SLOTS    = 10;
const int  NUMBER_OF_APP_INSTANCES = 10;

class request : public templet::message {
public:
	request(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) {}
    int    tag;
    list<long long> sextuplets;
    size_t ord;
};

/*event_log=array_of(tag,list_of(sextuplets))*/
vector<pair<int,list<long long>>> event_log;
int NUM_OF_RUNNING_APP_INSTANCES = NUMBER_OF_APP_INSTANCES;

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
        event_log.push_back(pair(m.tag,m.sextuplets));
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
        for(task_slots& slot:slots){
            slot.is_free = false; slot.is_ready = false; slot.sextuplets.clear();
            slot.from = slot.to = 0;
        }
        current_unchecked_num = 3;
        range_for_slot = SEARCH_RANGE_LIMIT / NUMBER_OF_SEARCH_RANGE_CHANKS;
        last_ready_tag = -1;
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
/*$TET$*/
	}

	inline void on_r(request&m) {
/*$TET$tcall$r*/
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$tcall$t*/
/*$TET$*/
	}

	request r;
	templet::basesim_task t;

/*$TET$tcall$$footer*/

long long current_unchecked_num;
long long range_for_slot;
int last_ready_tag;

struct task_slots{
    int  tag;
    bool is_free, is_ready;
    long long from, to;
    list<long long> sextuplets;
} slots[NUMBER_OF_TASK_SLOTS];

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

	eng.start();
	teng.run();

	if (!eng.stopped()) {
		cout << "Logical error" << std::endl; return;
	}

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
    run_prime_funcs_test();
    //run_state_sync_app();

    return EXIT_SUCCESS;
}
/*$TET$*/
