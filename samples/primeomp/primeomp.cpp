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

const bool CHECK_PRIMES = false;
const int  NUMBER_OF_FILTERS = 100;
const int  BLOCK_SIZE = 1000;
const int  NUMBER_OF_OUTPUTS = 100000;
const int  PRIME_TABLE_SIZE = 10;
//speed-up ~15.6

#include <templet.hpp>
#include <basesim.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <ctime>

class prime_candidate : public templet::message {
public:
	prime_candidate(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) { size = 0; }
	long number[BLOCK_SIZE];
	int  size;
};

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

bool maybe_prime_number(long num, long*table, int max_size)
{
	for (int i = 0; i < max_size; i++) {
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
/*$TET$*/

#pragma templet !source(out!prime_candidate)

struct source :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((source*)a)->on_out(*(prime_candidate*)m);}

	source(templet::engine&e) :source() {
		source::engines(e);
	}

	source() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$source$source*/
		number_to_check = 3;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$source$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$source$start*/
		on_out(out);
/*$TET$*/
	}

	inline void on_out(prime_candidate&m) {
/*$TET$source$out*/
		for (int i = m.size; i < BLOCK_SIZE; i++) {
			m.number[i] = number_to_check;
			number_to_check += 2;
		}
		m.size = BLOCK_SIZE;
		m.send();
/*$TET$*/
	}

	prime_candidate out;

/*$TET$source$$footer*/
	long number_to_check;
/*$TET$*/
};

#pragma templet mediator(in?prime_candidate,out!prime_candidate,t:basesim)

struct mediator :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((mediator*)a)->on_in(*(prime_candidate*)m);}
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((mediator*)a)->on_out(*(prime_candidate*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((mediator*)a)->on_t(*(templet::basesim_task*)t);}

	mediator(templet::engine&e,templet::basesim_engine&te_basesim) :mediator() {
		mediator::engines(e,te_basesim);
	}

	mediator() :templet::actor(false),
		out(this, &on_out_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$mediator$mediator*/
		_in = 0; prime_table_size = 0;
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$mediator$engines*/
/*$TET$*/
	}

	inline void on_in(prime_candidate&m) {
/*$TET$mediator$in*/
		_in = &m;
		if (access(out)) t.submit();
/*$TET$*/
	}

	inline void on_out(prime_candidate&m) {
/*$TET$mediator$out*/
		if (access(_in)) t.submit();
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$mediator$t*/
		auto start = std::chrono::high_resolution_clock::now();
		int input_size = _in->size;

		while (_in->size > 0 && out.size < BLOCK_SIZE ) {
			if (prime_table_size < PRIME_TABLE_SIZE) {
				if (!update_prime_table(_in->number[input_size - _in->size], prime_table, &prime_table_size, PRIME_TABLE_SIZE)) {
					out.number[out.size++] = _in->number[input_size - _in->size--];
				}
				else _in->size--;
			}
			else {
				if (maybe_prime_number(_in->number[input_size - _in->size], prime_table, PRIME_TABLE_SIZE)) {
					out.number[out.size++] = _in->number[input_size - _in->size--];
				}
				else _in->size--;
			}
		}

		if (_in->size == 0)_in->send();
		if (out.size != 0) out.send();

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end - start;
		t.delay(diff.count());
/*$TET$*/
	}

	void in(prime_candidate&m) { m.bind(this, &on_in_adapter); }
	prime_candidate out;
	templet::basesim_task t;

/*$TET$mediator$$footer*/
	prime_candidate* _in;
	int prime_table_size;
	long prime_table[PRIME_TABLE_SIZE];
/*$TET$*/
};

#pragma templet destination(in?prime_candidate)

struct destination :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((destination*)a)->on_in(*(prime_candidate*)m);}

	destination(templet::engine&e) :destination() {
		destination::engines(e);
	}

	destination() :templet::actor(false)
	{
/*$TET$destination$destination*/
		number_of_outputs = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$destination$engines*/
/*$TET$*/
	}

	inline void on_in(prime_candidate&m) {
/*$TET$destination$in*/
		if (number_of_outputs++ < NUMBER_OF_OUTPUTS) { m.size = 0; m.send(); }
		else stop();
/*$TET$*/
	}

	void in(prime_candidate&m) { m.bind(this, &on_in_adapter); }

/*$TET$destination$$footer*/
	int  number_of_outputs;
/*$TET$*/
};

/*$TET$$footer*/

mediator     an_intermediate_filter[NUMBER_OF_FILTERS];
source       a_source_filter;
destination  a_destination_filter;

int main()
{
	templet::engine eng;
    templet::basesim_engine teng;

    an_intermediate_filter[0].in(a_source_filter.out);
    
    for(int i=1; i<NUMBER_OF_FILTERS; i++ )       
        an_intermediate_filter[i].in(an_intermediate_filter[i-1].out);

    a_destination_filter.in(an_intermediate_filter[NUMBER_OF_FILTERS-1].out);
    
	a_source_filter.engines(eng);
	a_destination_filter.engines(eng);

	for (int i = 0; i < NUMBER_OF_FILTERS; i++)
		an_intermediate_filter[i].engines(eng, teng);
    
    eng.start();
    teng.run();

	if (eng.stopped()) {
		
		if (CHECK_PRIMES) {
			int size = PRIME_TABLE_SIZE * NUMBER_OF_FILTERS;
			long* table = new long[size];
			build_prime_table(table, size);
			int cur = 0;

			for (int i = 0; i < NUMBER_OF_FILTERS; i++)
				for (int j = 0; j < PRIME_TABLE_SIZE; j++)
					if (an_intermediate_filter[i].prime_table[j] != table[cur++]) {
						std::cout << "mismatch in an_intermediate_filter[" << i << "].prime_table[" << j << "]";
						return EXIT_FAILURE;
					}

			std::cout << "Check primes passed" << std::endl;
		}

		std::cout << "Maximum number of tasks executed in parallel : " << teng.Pmax() << std::endl;
		std::cout << "Time of sequential execution of all tasks    : " << teng.T1() << std::endl;
		std::cout << "Time of parallel   execution of all tasks    : " << teng.Tp() << std::endl;
		std::cout << "Speed-up                                     : " << teng.T1() / teng.Tp() << std::endl;
		
		return EXIT_SUCCESS;
	}

	std::cout << "something broke (((" << std::endl;
	return EXIT_FAILURE;
}
/*$TET$*/
