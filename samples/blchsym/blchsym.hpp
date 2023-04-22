/*$TET$$header*/
/*--------------------------------------------------------------------------*/
/*  Copyright 2023 Sergei Vostokin                                          */
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

#pragma once

#include <iostream>
#include <cstdlib>
#include <time.h>

#include <templet.hpp>
#include <basesim.hpp>

using namespace templet;
using namespace std;

//const int TASK_DELAY_IN_SEC = 10;
//const int NUM_WORKERS = 50;
//const int NUM_TASKS = 50;
//const unsigned RND_SEED = (unsigned)time(NULL);

int EXEC_ORDER[NUM_WORKERS][NUM_TASKS];

struct exec_order{
    static void define(){
        srand(RND_SEED);    
        for(int i=0; i<NUM_WORKERS; i++){
            for(int j=0; j<NUM_TASKS; j++){
redogen:                
                EXEC_ORDER[i][j] = rand()%NUM_TASKS;
                for(int k=0; k<j; k++) if(EXEC_ORDER[i][j]==EXEC_ORDER[i][k]) goto redogen;
            }
        }
    }
    static void print(){
        srand(time(NULL));    
        for(int i=0; i<NUM_WORKERS; i++){
            cout << "worker(" << i << ") plan:";
            for(int j=0; j<NUM_TASKS; j++){
                cout << " " << EXEC_ORDER[i][j];
            }
            cout << endl;
        }
    }
};

class request : public templet::message {
public:
	request(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
    // call
    int solved_task;
    // ret
    int num_solved;
    int solved_tasks[NUM_TASKS];  
};

/*$TET$*/

#pragma templet bchain(in?request)

struct bchain :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((bchain*)a)->on_in(*(request*)m);}

	bchain(templet::engine&e) :bchain() {
		bchain::engines(e);
	}

	bchain() :templet::actor(false)
	{
/*$TET$bchain$bchain*/
         num_solved = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$bchain$engines*/
/*$TET$*/
	}

	inline void on_in(request&m) {
/*$TET$bchain$in*/
        if(m.solved_task != -1){
            for(int i = 0; i<num_solved; i++)
                if(solved_tasks[i]==m.solved_task) goto go_on;
            
            solved_tasks[num_solved] = m.solved_task;
            num_solved++;
            
            // chain update
            cout << "bchain -> ";
            cout << "[" << solved_tasks[0];
            for(int i = 1; i<num_solved; i++) cout << ", " << solved_tasks[i];
            cout << "]" << endl; 
        }
go_on:        
        for(int i = 0; i<num_solved; i++) m.solved_tasks[i] = solved_tasks[i];
            m.num_solved = num_solved;
        
        m.send();
/*$TET$*/
	}

	void in(request&m) { m.bind(this, &on_in_adapter); }

/*$TET$bchain$$footer*/
    int num_solved;
    int solved_tasks[NUM_TASKS];  
/*$TET$*/
};

#pragma templet !bworker(out!request,ready!message,t:basesim)

struct bworker :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((bworker*)a)->on_out(*(request*)m);}
	static void on_ready_adapter(templet::actor*a, templet::message*m) {
		((bworker*)a)->on_ready(*(message*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((bworker*)a)->on_t(*(templet::basesim_task*)t);}

	bworker(templet::engine&e,templet::basesim_engine&te_basesim) :bworker() {
		bworker::engines(e,te_basesim);
	}

	bworker() :templet::actor(true),
		out(this, &on_out_adapter),
		ready(this, &on_ready_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$bworker$bworker*/
        cur_task = 0;
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$bworker$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$bworker$start*/
        out.solved_task = -1;
        out.num_solved = 0;
        out.send();
/*$TET$*/
	}

	inline void on_out(request&m) {
/*$TET$bworker$out*/
        bool solved;
        do{         
            cur_task_temp = EXEC_ORDER[worker_ID][cur_task];
            solved = false;
            
            for(int i=0; i<out.num_solved; i++){
                if(out.solved_tasks[i] == cur_task_temp){
                    solved = true; break;
                }
            }
                
            cur_task++;    
        }while(solved && cur_task<NUM_TASKS);
        
        if (solved) ready.send(); else t.submit();    
/*$TET$*/
	}

	inline void on_ready(message&m) {
/*$TET$bworker$ready*/
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$bworker$t*/     
        t.delay(TASK_DELAY_IN_SEC); // do task
        cout << "worker(" << worker_ID <<") -> " << cur_task_temp << endl;  
            
        out.solved_task = cur_task_temp;
        out.send();
 /*$TET$*/
	}

	request out;
	message ready;
	templet::basesim_task t;

/*$TET$bworker$$footer*/
    int cur_task_temp;
    int cur_task;
    int worker_ID;
/*$TET$*/
};

#pragma templet stopper(in?message)

struct stopper :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((stopper*)a)->on_in(*(message*)m);}

	stopper(templet::engine&e) :stopper() {
		stopper::engines(e);
	}

	stopper() :templet::actor(false)
	{
/*$TET$stopper$stopper*/
        num_stopped = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$stopper$engines*/
/*$TET$*/
	}

	inline void on_in(message&m) {
/*$TET$stopper$in*/
        
        if(++num_stopped == num_total) stop();
/*$TET$*/
	}

	void in(message&m) { m.bind(this, &on_in_adapter); }

/*$TET$stopper$$footer*/
    int num_stopped;
    int num_total;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
