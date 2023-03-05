/*$TET$$header*/
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

#include <templet.hpp>
#include <everest.hpp>

// reset to the number of available resources
const int NUM_WORKERS = 10;

class request : public templet::message {
public:
	request(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
	bool is_first;

/***** task input *****/
/**/	double rho, sigma, beta;
/***** task output *****/
/**/	double L1, L2, L3;
};
/***** problem state *****/
/**/	int NUM_TASKS;
/**/
/**/	double *rho;
/**/	double *sigma;
/**/	double *beta;
/**/
/**/	int current = 0;
/**/    double the_best_rho, the_best_sigma, the_best_beta;
/**/    double the_best_L1=0.0, the_best_L2=0.0, the_best_L3=0.0;
/*************/

	string app_ID;
	string maple_code;

/*$TET$*/

#pragma templet !worker(r!request,t:everest)

struct worker :public templet::actor {
	static void on_r_adapter(templet::actor*a, templet::message*m) {
		((worker*)a)->on_r(*(request*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((worker*)a)->on_t(*(templet::everest_task*)t);}

	worker(templet::engine&e,templet::everest_engine&te_everest) :worker() {
		worker::engines(e,te_everest);
	}

	worker() :templet::actor(true),
		r(this, &on_r_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$worker$worker*/
		t.app_id(app_ID.c_str());
/*$TET$*/
	}

	void engines(templet::engine&e,templet::everest_engine&te_everest) {
		templet::actor::engine(e);
		t.engine(te_everest);
/*$TET$worker$engines*/
        teng = &te_everest;
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
		json in;
        string maple_code_uri;
		in["name"] = "Lyapunov-exp";
        teng->upload(maple_code, maple_code_uri);
		in["inputs"]["code"] = maple_code_uri;
		in["inputs"]["r"] = m.rho;
		in["inputs"]["s"] = m.sigma;
		in["inputs"]["b"] = m.beta;
		t.submit(in);
/*$TET$*/
	}

	inline void on_t(templet::everest_task&t) {
/*$TET$worker$t*/
		json& out = t.result();
		r.L1 = out["L1"]; r.L2 = out["L2"]; r.L3 = out["L3"];
		r.send();
/*$TET$*/
	}

	request r;
	templet::everest_task t;

/*$TET$worker$$footer*/
    templet::everest_engine*teng;
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
		else {
/***** do it when the task is ready *****/
/**/		cout << "{rho=" << m.rho << ";sigma=" << m.sigma << ";beta=" << m.beta << "}->"
/**/				"{L1=" << m.L1 << ";L2=" << m.L2 << ";L3=" << m.L3 << "}" << endl;
/**/		if(m.L1 > the_best_L1){
/**/  			the_best_L1 = m.L1; the_best_L2 = m.L2; the_best_L3 = m.L3;
/**/   			the_best_rho = m.rho; the_best_sigma = m.sigma; the_best_beta = m.beta;
/**/		}
/**/		cout << "Currently the best chaotic behavior is" << endl;
/**/		cout << "{rho=" << the_best_rho << ";sigma=" << the_best_sigma << ";beta=" << the_best_beta << "}->"
/**/				"{L1=" << the_best_L1 << ";L2=" << the_best_L2 << ";L3=" << the_best_L3 << "}" << endl;           /*************/
		}
		req_list.push_back(&m);

		
		while (!req_list.empty() &&
/***** check if we have a task *****/
/**/		current < NUM_TASKS
/*************/
			) {
			request* r = req_list.front();
			req_list.pop_front();
/***** form a new task *****/
/**/		r->rho   = rho[current];
/**/		r->sigma = sigma[current];
/**/		r->beta  = beta[current];
/**/		current++;
/*************/
			r->send();
		}
		if (req_list.size() == NUM_WORKERS)	stop();
/*$TET$*/
	}

	void r(request&m) { m.bind(this, &on_r_adapter); }

/*$TET$master$$footer*/
	list<request*> req_list;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
