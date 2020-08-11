/*--------------------------------------------------------------------------*/
/*  Copyright 2020 Sergei Vostokin                                          */
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

#include <vector>
#include <queue>
#include <cassert>

#include "templet.hpp"

namespace templet {

	class basesim_task;

	struct event{
		enum { TASK_BEGIN, TASK_END } _type;
		double _time;
		basesim_task* _task;
	};

	class cmp { public: bool operator()(const event&t1, const event&t2) { return t1._time > t2._time; } };

	class basesim_engine {
		friend class basesim_task;
	public:
		basesim_engine() {
			_recount = 0; _task_mode = false; init();
		}

		void run() {
			assert(++_recount == 1);
			while (!_calendar.empty()) {
				event ev = _calendar.top();	_calendar.pop();
				_wait_loop_body(ev);
			}
			assert(--_recount== 0);
		}

		double T1() {  return _T1; }
		double Tp() {  return _Tp; }
		int Pmax()  {  return _Pmax; }

	private:
		void delay(double t) { assert(_task_mode); _cur_delay += t;	_T1 += t; }
		void submit(basesim_task&t);

	private:
		inline void _wait_loop_body(event&ev);
		inline void init();

	private:
		std::priority_queue<event, std::vector<event>, cmp> _calendar;
		double _Tp;
		double _T1;
		int _Pmax;

		int    _recount;
		bool   _task_mode;
		basesim_task*  _cur_task;
		double _cur_delay;
		int    _cur_P;
	};

	class basesim_task : task {
		friend	class basesim_engine;
	public:
		basesim_task(actor*a, task_adaptor ta) : _actor(a), _tsk_adaptor(ta), _eng(0), _is_idle(true) {}
		void engine(basesim_engine&e) { assert(_eng == 0); _eng = &e; }
		
		void delay(double t) { _eng->delay(t); }
		void submit() { _actor->suspend(); _eng->submit(*this); }

	private:
		actor*       _actor;
		task_adaptor _tsk_adaptor;

		basesim_engine* _eng;
		bool _is_idle;
	};

	void basesim_engine::init() {
		assert(_recount == 0);
		_Tp = 0; _T1 = 0; _Pmax = 0; _cur_P = 0;
		while (!_calendar.empty()) { 
			_calendar.top()._task->_is_idle = true;
			_calendar.pop(); 
		}
	}

	void basesim_engine::submit(basesim_task&t) {
		assert(t._eng == this && t._is_idle && !_task_mode);
	
		event ev;
		ev._time = _Tp;
		ev._type = event::TASK_BEGIN;
		ev._task = &t; t._is_idle = false;
		_calendar.push(ev);
	}

	void basesim_engine::_wait_loop_body(event&ev) {
		basesim_task*t = ev._task;  _Tp = ev._time;

		if (ev._type == event::TASK_BEGIN) {
			
			if (++_cur_P > _Pmax) _Pmax = _cur_P;

			_task_mode = true;
			_cur_delay = 0.0;
			(*t->_tsk_adaptor)(t->_actor, t);
			_task_mode = false;

			event new_ev;
			new_ev._time = _Tp + _cur_delay;
			new_ev._type = event::TASK_END;
			new_ev._task = t; t->_is_idle = false;
			
			_calendar.push(new_ev);
		}
		else if (ev._type == event::TASK_END) {

			_cur_P--;

			t->_is_idle = true;
			t->_actor->resume();
		}
	}
}
