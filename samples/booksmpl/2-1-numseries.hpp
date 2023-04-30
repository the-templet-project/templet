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

#include <templet.hpp>
#include <iostream>

using namespace templet;
using namespace std;

const int NO_RANGE_ID = -1;

class range : public templet::message {
public:
	range(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
	int range_id;
    double result;
};


/*$TET$*/

#pragma templet Master(in?range)

struct Master :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Master*)a)->on_in(*(range*)m);}

	Master(templet::engine&e) :Master() {
		Master::engines(e);
	}

	Master() :templet::actor(false)
	{
/*$TET$Master$Master*/
        current_range = 0;
        num_of_ready_workers = 0;
        PI = 0.0;        
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Master$engines*/
/*$TET$*/
	}

	inline void on_in(range&m) {
/*$TET$Master$in*/
        if(m.range_id == NO_RANGE_ID){
            m.range_id = current_range++;
            m.send();
        }
        else{
            PI = PI + m.result;
            num_of_ready_workers++;
            if(num_of_ready_workers == N) stop();
        }
/*$TET$*/
	}

	void in(range&m) { m.bind(this, &on_in_adapter); }

/*$TET$Master$$footer*/
    int current_range;
    int num_of_ready_workers;
    double PI;    
/*$TET$*/
};

#pragma templet !Worker(out!range)

struct Worker :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Worker*)a)->on_out(*(range*)m);}

	Worker(templet::engine&e) :Worker() {
		Worker::engines(e);
	}

	Worker() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$Worker$Worker*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Worker$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Worker$start*/
        out.range_id = NO_RANGE_ID;
        out.send();
/*$TET$*/
	}

	inline void on_out(range&m) {
/*$TET$Worker$out*/
        double sum = 0.0;           
        for (long i=m.range_id; i < num_steps; i=i+N) {
             double x = (i+0.5)*step;
             sum += 4.0/(1.0+x*x);
        }
        m.result = sum*step;
        m.send();
/*$TET$*/
	}

	range out;

/*$TET$Worker$$footer*/
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
