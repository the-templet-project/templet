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

const long long SEARCH_RANGE_LIMIT = 100000000;
const int  NUMBER_OF_TASK_SLOTS = 10;
const int  NUMBER_OF_WORKERS    = 10;

struct task_slots{
    bool is_free;
    bool is_ready;
    list<long long> sextuples;
} slots[NUMBER_OF_TASK_SLOTS] = {0};

class request : public templet::message {
public:
	request(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) {}
	bool is_first;
    int  slot_num;
};


/*$TET$*/

#pragma templet !worker(r!request,t:basesim)

struct worker :public templet::actor {
	static void on_r_adapter(templet::actor*a, templet::message*m) {
		((worker*)a)->on_r(*(request*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((worker*)a)->on_t(*(templet::basesim_task*)t);}

	worker(templet::engine&e,templet::basesim_engine&te_basesim) :worker() {
		worker::engines(e,te_basesim);
	}

	worker() :templet::actor(true),
		r(this, &on_r_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$worker$worker*/
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$worker$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$worker$start*/
/*$TET$*/
	}

	inline void on_r(request&m) {
/*$TET$worker$r*/
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$worker$t*/
/*$TET$*/
	}

	request r;
	templet::basesim_task t;

/*$TET$worker$$footer*/
/*$TET$*/
};

#pragma templet master(r?request)

struct master :public templet::actor {
	static void on_r_adapter(templet::actor*a, templet::message*m) {
		((master*)a)->on_r(*(request*)m);}

	master(templet::engine&e) :master() {
		master::engines(e);
	}

	master() :templet::actor(false)
	{
/*$TET$master$master*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$master$engines*/
/*$TET$*/
	}

	inline void on_r(request&m) {
/*$TET$master$r*/
/*$TET$*/
	}

	void r(request&m) { m.bind(this, &on_r_adapter); }

/*$TET$master$$footer*/
/*$TET$*/
};

/*$TET$$footer*/
int main()
{
    list<long long> table;

    // 1 case
    auto start = std::chrono::high_resolution_clock::now();
	sextuplets_search(table,SEARCH_RANGE_LIMIT);
	auto end = std::chrono::high_resolution_clock::now();
    
	std::chrono::duration<double> diff = end - start;	
    cout << "sequential exec. time = " << diff.count() << "s" << endl;

    // 2 case
    table.clear();
    std::list<long long> found;

    start = std::chrono::high_resolution_clock::now();
    
    long long max_num_in_table = (long long)sqrt(SEARCH_RANGE_LIMIT);

    cout << "max_num_in_table = " << max_num_in_table << endl;
    
    extend_prime_table(table, 3, max_num_in_table);
    
    find_sextuplets_in_range(table,3,SEARCH_RANGE_LIMIT,found);
 
    end = std::chrono::high_resolution_clock::now();
    
    diff = end - start;	
    cout << "sequential exec. time = " << diff.count() << "s" << endl;

    return EXIT_SUCCESS;
}
/*$TET$*/
