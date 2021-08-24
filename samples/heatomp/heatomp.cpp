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

const int NUMBER_OF_STAGES = 10;
const int NUMBER_OF_ITERATIONS = 1000;

const int COLUMNS = 1000; // sequential
const int ROWS    = 1000; // parallel

const bool DO_CHECK = true;
double field[ROWS][COLUMNS];
double test_field[ROWS][COLUMNS];

//#define SIMULATION
//#define TEMPLET_CPP_SYNC
#define TEMPLET_OMP_SYNC
//#define SIMPLE_RESUME

#include <templet.hpp>
#include <omp.h>

#ifdef SIMULATION
#include <basesim.hpp>
#else
#include <omptask.hpp>
#endif

#include <chrono>
#include <cmath>
#include <iostream>
#include <ctime>

class signal : public templet::message {
public:
	signal(templet::actor*a = 0, templet::message_adaptor ma = 0) :templet::message(a, ma) {}
};

void initial_field(double field[ROWS][COLUMNS])
{
	srand(0);
	for (int i = 0; i < ROWS; i++) 
		for (int j = 0; j < COLUMNS; j++) 
			field[i][j] = (double)(rand() % 100);
}

void test_computation() {
	for (int t = 0; t < NUMBER_OF_ITERATIONS; t++) 
		for (int i = 1; i < ROWS - 1; i++)
			for (int j = 1; j < COLUMNS - 1; j++) 
				test_field[i][j] = (test_field[i-1][j]+ test_field[i+1][j]+ test_field[i][j-1]+ test_field[i][j+1])*0.25;
}

bool computations_are_equal()
{
	for (int i = 1; i < ROWS - 1; i++)
		for (int j = 1; j < COLUMNS - 1; j++)
			if (field[i][j] != test_field[i][j]) return false;
	return true;
}

void one_iter_of_stage(int n)
{
	int size = (ROWS - 2) / NUMBER_OF_STAGES + (n + 1 <= (ROWS - 2) % NUMBER_OF_STAGES ? 1 : 0);
	int addon = ((n + 1) <= (ROWS - 2) % NUMBER_OF_STAGES ? (n + 1) : (ROWS - 2) % NUMBER_OF_STAGES);
	int end = (ROWS - 2) / NUMBER_OF_STAGES * (n + 1) + addon;
	int beg = end - size + 1;
	
	for (int i = beg; i <= end; i++)
		for (int j = 1; j < COLUMNS - 1; j++)
			field[i][j] = (field[i - 1][j] + field[i + 1][j] + field[i][j - 1] + field[i][j + 1])*0.25;
}

/*$TET$*/

#pragma templet !starter(out!signal)

struct starter :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((starter*)a)->on_out(*(signal*)m);}

	starter(templet::engine&e) :starter() {
		starter::engines(e);
	}

	starter() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$starter$starter*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$starter$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$starter$start*/
		on_out(out);
/*$TET$*/
	}

	inline void on_out(signal&m) {
/*$TET$starter$out*/
		m.send();
/*$TET$*/
	}

	signal out;

/*$TET$starter$$footer*/
/*$TET$*/
};

#pragma templet stage(in?signal,out!signal,t:omptask)

struct stage :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((stage*)a)->on_in(*(signal*)m);}
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((stage*)a)->on_out(*(signal*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((stage*)a)->on_t(*(templet::omptask_task*)t);}

	stage(templet::engine&e,templet::omptask_engine&te_omptask) :stage() {
		stage::engines(e,te_omptask);
	}

	stage() :templet::actor(false),
		out(this, &on_out_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$stage$stage*/
		_in = 0; iters_done = 0;
/*$TET$*/
	}

	void engines(templet::engine&e,templet::omptask_engine&te_omptask) {
		templet::actor::engine(e);
		t.engine(te_omptask);
/*$TET$stage$engines*/
/*$TET$*/
	}

	inline void on_in(signal&m) {
/*$TET$stage$in*/
		_in = &m;
		if (access(out) && iters_done < NUMBER_OF_ITERATIONS) {
			t.submit(); iters_done++;
		}
/*$TET$*/
	}

	inline void on_out(signal&m) {
/*$TET$stage$out*/
		if (access(_in) && iters_done < NUMBER_OF_ITERATIONS) {
			t.submit();	iters_done++;
		}
/*$TET$*/
	}

	inline void on_t(templet::omptask_task&t) {
/*$TET$stage$t*/
#ifdef SIMULATION
		double T = omp_get_wtime();
#endif		
		one_iter_of_stage(stage_ID);

#ifdef SIMULATION		
		T = omp_get_wtime() - T;
		t.delay(T);
#endif
		out.send();	_in->send();
/*$TET$*/
	}

	void in(signal&m) { m.bind(this, &on_in_adapter); }
	signal out;
	templet::omptask_task t;

/*$TET$stage$$footer*/
	signal* _in;
	int iters_done;
	int stage_ID;
/*$TET$*/
};

#pragma templet stopper(in?signal)

struct stopper :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((stopper*)a)->on_in(*(signal*)m);}

	stopper(templet::engine&e) :stopper() {
		stopper::engines(e);
	}

	stopper() :templet::actor(false)
	{
/*$TET$stopper$stopper*/
		iters_done = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$stopper$engines*/
/*$TET$*/
	}

	inline void on_in(signal&m) {
/*$TET$stopper$in*/
		if (++iters_done < NUMBER_OF_ITERATIONS) m.send(); else stop();
/*$TET$*/
	}

	void in(signal&m) { m.bind(this, &on_in_adapter); }

/*$TET$stopper$$footer*/
	int iters_done;
/*$TET$*/
};

/*$TET$$footer*/
int main()
{
	stage   a_stage[NUMBER_OF_STAGES];
	starter a_starter;
	stopper a_stopper;
			
	templet::engine eng;
#ifdef SIMULATION
	templet::basesim_engine teng;
#else
	templet::omptask_engine teng;
#endif

	a_stage[0].in(a_starter.out);
	a_stage[0].stage_ID = 0;

	for (int i = 1; i < NUMBER_OF_STAGES; i++) {
		a_stage[i].in(a_stage[i - 1].out);
		a_stage[i].stage_ID = i;
	}

	a_stopper.in(a_stage[NUMBER_OF_STAGES - 1].out);

	a_starter.engines(eng);
	a_stopper.engines(eng);

	for (int i = 0; i < NUMBER_OF_STAGES; i++)
		a_stage[i].engines(eng, teng);

	initial_field(field);

#ifndef SIMULATION
	double Tp = omp_get_wtime();
#endif 

	eng.start();
	teng.run();

#ifndef SIMULATION
	Tp = omp_get_wtime() - Tp;
#endif 

	if (eng.stopped()) {
#ifdef SIMULATION
		initial_field(test_field);
		test_computation();

		if (!computations_are_equal()) { std::cout << "Check failed!!!" << std::endl; return EXIT_FAILURE; }
		std::cout << "Maximum number of tasks executed in parallel : " << teng.Pmax() << std::endl;
		std::cout << "Time of sequential execution of all tasks    : " << teng.T1() << std::endl;
		std::cout << "Time of parallel   execution of all tasks    : " << teng.Tp() << std::endl;
		std::cout << "Speed-up                                     : " << teng.T1() / teng.Tp() << std::endl;
#else
		initial_field(test_field);
		double T1 = omp_get_wtime();
		test_computation();
		T1 = omp_get_wtime() - T1;

		if (!computations_are_equal()) { std::cout << "Check failed!!!" << std::endl; return EXIT_FAILURE; }
		std::cout << "Maximum number of tasks executed in parallel : " << omp_get_num_procs() << std::endl;
		std::cout << "Time of sequential execution of all tasks    : " << T1 << std::endl;
		std::cout << "Time of parallel   execution of all tasks    : " << Tp << std::endl;
		std::cout << "Speed-up                                     : " << T1 / Tp << std::endl;
#endif		
		return EXIT_SUCCESS;
	}
	else std::cout << "something broke (((" << std::endl;
	return EXIT_FAILURE;
}
/*$TET$*/
