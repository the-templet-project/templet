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

// complexity of calculation
const long long SEARCH_RANGE_LIMIT       = 100000000;
const int  NUMBER_OF_SEARCH_RANGE_CHANKS = 1000;

// parallelism / recalculations 
const int  NUMBER_OF_TASK_SLOTS    = 100; // mast be less then NUMBER_OF_SEARCH_RANGE_CHANKS
const int  NUMBER_OF_APP_INSTANCES = 100;

void chank_tag_to_range(int tag,long long& from,long long& to){
    const long long CHANK_SIZE = SEARCH_RANGE_LIMIT / NUMBER_OF_SEARCH_RANGE_CHANKS; 
    from = tag * CHANK_SIZE;
    to = (tag < NUMBER_OF_SEARCH_RANGE_CHANKS-1) ? from + CHANK_SIZE - 1 : SEARCH_RANGE_LIMIT;
    if(from-15 > 2) from-=15;
    if(from==0)     from=3;
}

long long chank_tag_to_max_verif_prime(int tag){
    const long long CHANK_SIZE = SEARCH_RANGE_LIMIT / NUMBER_OF_SEARCH_RANGE_CHANKS; 
    long long to = (tag < NUMBER_OF_SEARCH_RANGE_CHANKS-1) ? tag * CHANK_SIZE + CHANK_SIZE : SEARCH_RANGE_LIMIT;
    return sqrt((double)to)+1; 
}

class request : public templet::message {
public:
	request(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) {}
    bool   input;              //is tag/sextuplets pair valid?
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
        prime_table.clear();
        prime_limit = 3;
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
        auto start = std::chrono::high_resolution_clock::now();   
        
        for(int i=task_ord_to_start_update_from; i<=r.ord; i++){
            if(task_selector.task_ready(event_log[i].first))
                task_sorter.ready.push_back(pair(event_log[i].first,event_log[i].second));
        }
        task_sorter.print_ready();
        task_ord_to_start_update_from = r.ord+1;

        int task_tag;
        if(task_selector.select_task(task_tag)){
            // process task
            r.tag = task_tag; r.sextuplets.clear();
            
            long long max_prime_needed = chank_tag_to_max_verif_prime(task_tag); 
            if(prime_limit < max_prime_needed) prime_limit = extend_prime_table(prime_table,prime_limit,max_prime_needed);
            
            long long from, to;
            chank_tag_to_range(task_tag,from,to);
            find_sextuplets_in_range(prime_table,from,to,r.sextuplets);
            
            r.input = true;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;
            t.delay(diff.count());
        } 
        else{
            r.input = false;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;
            t.delay(diff.count()); 
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
    std::list<long long> prime_table;
    long long prime_limit;

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
        last_completed_task_tag=-1;
        ready.clear();
        
        print_enabled = false;
        last_printed_sextuplet=0;
    } 
    void print_ready(){
        ready.sort([](const auto& l, const auto& r){return l.first < r.first;});
        
        for(auto it = ready.begin(); it != ready.end();){
            if(last_completed_task_tag+1==it->first){
                last_completed_task_tag++;
                
                if(print_enabled){
                    long long from,to;
                    chank_tag_to_range(last_completed_task_tag,from,to);
                    cout<<"task tag #" << last_completed_task_tag << " in range(" << from << "," << to << ") prime_limit="
                        << chank_tag_to_max_verif_prime(last_completed_task_tag) << endl;
                    
                    for(auto n:it->second) cout<<"sextuplet #" << ++last_printed_sextuplet << " ("<< n <<")" << endl;
                }
                it = ready.erase(it);
            }
            else break;
        }
    }
    bool completed(){return last_completed_task_tag==NUMBER_OF_SEARCH_RANGE_CHANKS-1;}
    
    int last_completed_task_tag;
    
    bool print_enabled;
    int last_printed_sextuplet;
    list<pair<int,list<long long>>> ready;
} task_sorter;

/*$TET$*/
};

/*$TET$$footer*/

double run_parallel()
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

    cout << "==== parallel operation simulation ====" << endl;
    
	eng.start();
	teng.run();

	if (!eng.stopped()) {cout << "Logical error" << std::endl; return 0.0;}

    cout << endl;
	cout << "Maximum number of tasks executed in parallel : " << teng.Pmax() << endl;
	cout << "Time of sequential execution of all tasks    : " << teng.T1() << "s" << endl;
	cout << "Time of parallel   execution of all tasks    : " << teng.Tp() << "s" << endl;
	cout << "Estimated speedup                            : " << teng.T1() / teng.Tp() << std::endl;

    return teng.Tp();
}

double run_sequential()
{
    list<long long> table;

    cout << endl << "==== sequential operation ====" << endl;
        
    auto start = std::chrono::high_resolution_clock::now();
	sextuplets_search(table,SEARCH_RANGE_LIMIT);
	auto end = std::chrono::high_resolution_clock::now();
    
	std::chrono::duration<double> diff = end - start;	
    cout << endl <<"Time of sequential execution : " << diff.count() << "s" << endl << endl;

    return diff.count();
}

int main()
{
    double T1, Tp;
    T1 = run_sequential();
    Tp = run_parallel();

    cout << endl <<"Expected absolute speedup : " << T1/Tp << endl;

    return EXIT_SUCCESS;
}
/*$TET$*/
