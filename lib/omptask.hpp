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

#pragma once

#include <cassert>
#include <queue>
#include <omp.h>

#include "templet.hpp"

using namespace std;

namespace templet {
	class omptask_task;
	
	class omptask_engine {
		friend	class omptask_task;
	public:
		omptask_engine(): _num_active(0) {omp_init_lock(&_queue_lock);}
		~omptask_engine() {omp_destroy_lock(&_queue_lock);}
		void run();
	private:
		void submit(omptask_task*t);
	private:
		int  _num_active;
		queue<omptask_task*> _queue;
		omp_lock_t _queue_lock;
	};

	class omptask_task : task {
		friend	class omptask_engine;
	public:
		omptask_task(actor*a, task_adaptor ta) : _actor(a), _tsk_adaptor(ta), _eng(0), _idle(true){}
		void engine(omptask_engine&e) { assert(_eng == 0); _eng = &e; }
		void submit() { assert(_idle); _idle = false; _actor->suspend(); _eng->submit(this); }
	protected:
		virtual void run() {}
	private:
		bool         _idle;
		actor*       _actor;
		task_adaptor _tsk_adaptor;
		omptask_engine* _eng;
	};

	void omptask_engine::submit(omptask_task*t) {
		omp_set_lock(&_queue_lock);
		_queue.push(t);
		omp_unset_lock(&_queue_lock);
	}
	
	void omptask_engine::run(){
		bool goon = true;
		bool is_active = false;

#pragma omp parallel firstprivate(goon,is_active)
		{
			while (goon) {
				omp_set_lock(&_queue_lock);

				if (is_active) { is_active = false; _num_active--; }
				
				if (_queue.empty()) {
					if (_num_active == 0) goon = false;
					omp_unset_lock(&_queue_lock);
				}
				else {
					omptask_task* tsk = _queue.front(); _queue.pop();
					_num_active++; is_active = true;
					omp_unset_lock(&_queue_lock);
					
					tsk->run();
					(*tsk->_tsk_adaptor)(tsk->_actor, tsk);
					tsk->_idle = true;					

					tsk->_actor->resume();
				}
			}
		}
	}
}
