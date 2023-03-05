/*$TET$$header*/
/*--------------------------------------------------------------------------*/
/*  Copyright 2021 Sergei Vostokin                                          */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/

#include <chrono>
#include <cmath>
#include <iostream>
#include <ctime>
#include <cassert>
#include <list>
#include <climits>

#include <templet.hpp>
#include <basesim.hpp>

const int PRIME_BUFF_SIZE = 100;
const int PRIME_TABLE_SIZE = 1000000;
const int NUMBER_OF_TASK_SLOTS = 10;
const int NUM_WORKERS = 10;

#define DO_TEST

#ifdef DO_TEST
long test_prime[PRIME_TABLE_SIZE];
#endif 

int  global_prime_size = 0;
long global_prime[PRIME_TABLE_SIZE];

int  awaiting_task_id = 0;
int  free_task_id = 0;
int  number_of_free_slots = NUMBER_OF_TASK_SLOTS;

long cur_number_to_check = 3;
long max_number_to_check = 3;

struct task_slot {
	bool is_free;
	bool is_ready;
	long num_to_scan_from;
	long num_to_scan_to;
	int  task_id;
	int  global_prime_size;
	int  prime_size;
	long prime[PRIME_BUFF_SIZE];
} slot[NUMBER_OF_TASK_SLOTS] = {0};

void init_bag_of_tasks()
{
	global_prime_size = 0;
	awaiting_task_id = 0;
	free_task_id = 0;
	number_of_free_slots = NUMBER_OF_TASK_SLOTS;
	cur_number_to_check = 3;
	max_number_to_check = 3;

	for (int i = 0; i < NUMBER_OF_TASK_SLOTS; i++) { 
		slot[i].is_free = true; 
		slot[i].is_ready = false; 
	}
}

bool any_task_in_bag()
{
	return (global_prime_size < PRIME_TABLE_SIZE &&
		number_of_free_slots > 0 &&
		cur_number_to_check <= max_number_to_check);
}

void get_task_from_bag(int*task_slot)
{
	int i;
	for (i = 0; i < NUMBER_OF_TASK_SLOTS; i++) if (slot[i].is_free) break;
	
	assert (i != NUMBER_OF_TASK_SLOTS);

	slot[i].is_free  = false;
	slot[i].is_ready = false;
	number_of_free_slots--;

	long nums_in_range = (max_number_to_check - cur_number_to_check) / 2 + 1;
	long num_to_scan_to = cur_number_to_check + (PRIME_BUFF_SIZE - 1) * 2;

	slot[i].num_to_scan_from = cur_number_to_check;
	slot[i].num_to_scan_to = (nums_in_range <= PRIME_BUFF_SIZE ? max_number_to_check : num_to_scan_to);

	cur_number_to_check = slot[i].num_to_scan_to + 1;
	if (cur_number_to_check % 2 == 0)cur_number_to_check++;
	
	slot[i].task_id = free_task_id++;
	slot[i].global_prime_size = global_prime_size;
	slot[i].prime_size = 0;

	*task_slot = i;
}

void put_task_to_bag(int task_slot)
{
	assert(!slot[task_slot].is_free && !slot[task_slot].is_ready);

	slot[task_slot].is_ready = true;

	if (slot[task_slot].task_id == awaiting_task_id) {

copy_primes:		
		int i, j;
		for (i = 0, j = global_prime_size;
			i < slot[task_slot].prime_size && j < PRIME_TABLE_SIZE;
			i++, j++) {
			global_prime[j] = slot[task_slot].prime[i];
		}

		global_prime_size = j;
		awaiting_task_id++;
		slot[task_slot].is_free = true;
		number_of_free_slots++;

		for (int n = 0; n < NUMBER_OF_TASK_SLOTS; n++) {
			if (!slot[n].is_free && slot[n].is_ready && slot[n].task_id == awaiting_task_id) {
				task_slot = n;
				goto copy_primes;
			}
		}

		long last_prime = global_prime[global_prime_size - 1];
		
		if (last_prime <= 46430) { 
			max_number_to_check = last_prime * last_prime; 
			if (max_number_to_check % 2 == 0) max_number_to_check--;
		}
		else max_number_to_check = LONG_MAX;
	}
}

bool is_prime_number(long num, long*table, int size);

void process_task(int task_slot)
{
	assert(!slot[task_slot].is_free && !slot[task_slot].is_ready);

	long num  = slot[task_slot].num_to_scan_from;
	long last = slot[task_slot].num_to_scan_to;
	int  size = slot[task_slot].global_prime_size;

	int i = 0;
	while (i < PRIME_BUFF_SIZE && num <= last) {
		if (is_prime_number(num, global_prime, size)) slot[task_slot].prime[i++] = num;
		num += 2;
	}

	assert(num >= last);

	slot[task_slot].prime_size = i;
}
//////////////////////////////////////////////////////////////////////////
bool update_prime_table(long num, long*table, int*cur_size, int max_size)
{
	for (int i = 0; i < *cur_size; i++) {
		long divisor = table[i];
		std::ldiv_t dv = std::div(num, divisor);
		if (dv.rem == 0) return true;
		if (dv.quot <= divisor) break;
	}

	if (*cur_size < max_size) {
		table[(*cur_size)++] = num;
		return true;
	}

	return false;
}

bool is_prime_number(long num, long*table, int size)
{
	for (int i = 0; i < size; i++) {
		long divisor = table[i];
		std::ldiv_t dv = std::div(num, divisor);
		if (dv.rem == 0) return false;
		if (dv.quot <= divisor) break;
	}

	return true;
}

void build_prime_table(long*table, int size)
{
	long n = 3; int cur_size = 0;
	while (update_prime_table(n, table, &cur_size, size))n += 2;
}
//////////////////////////////////////////////////////////////////////////
class request : public templet::message {
public:
	request(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) {}
	bool is_first;
	int task_slot;
};
//////////////////////////////////////////////////////////////////////////
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
		r.is_first = true;
		r.send();
/*$TET$*/
	}

	inline void on_r(request&m) {
/*$TET$worker$r*/
		t.submit();
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$worker$t*/
		auto start = std::chrono::high_resolution_clock::now();
		process_task(r.task_slot);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end - start;
		t.delay(diff.count());
		r.send();
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
		if (m.is_first) m.is_first = false;
		else put_task_to_bag(m.task_slot);
		
		req_list.push_back(&m);

		while (!req_list.empty() && any_task_in_bag()) {
			request* r = req_list.front();
			req_list.pop_front();
			get_task_from_bag(&(r->task_slot));
			r->send();
		}
		if (req_list.size() == NUM_WORKERS)	stop();
/*$TET$*/
	}

	void r(request&m) { m.bind(this, &on_r_adapter); }

/*$TET$master$$footer*/
	std::list<request*> req_list;
/*$TET$*/
};

/*$TET$$footer*/
int main()
{
	templet::engine eng;
	templet::basesim_engine teng;

	master mst(eng);
	worker wks[NUM_WORKERS];

	for (int i = 0; i < NUM_WORKERS; i++) {
		wks[i].engines(eng,teng);
		mst.r(wks[i].r);
	}

	init_bag_of_tasks();

	srand(time(NULL));

	eng.start();
	teng.run();

	if (!eng.stopped()) {
		std::cout << "Logical error" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Maximum number of tasks executed in parallel : " << teng.Pmax() << std::endl;
	std::cout << "Time of sequential execution of all tasks    : " << teng.T1() << std::endl;
	std::cout << "Time of parallel   execution of all tasks    : " << teng.Tp() << std::endl;
	std::cout << "Estimated speed-up                           : " << teng.T1() / teng.Tp() << std::endl;

#ifdef DO_TEST
	build_prime_table(test_prime, PRIME_TABLE_SIZE);
	
	for (int i = 0; i < PRIME_TABLE_SIZE; i++)
		if (test_prime[i] != global_prime[i]) {
			std::cout << "Test failed at i = " << i; 
			return EXIT_FAILURE;
		}

	std::cout << "Test passed";
#endif

	return EXIT_SUCCESS;
}
/*$TET$*/
