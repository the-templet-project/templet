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
#include <list>

using namespace templet;
using namespace std;

class range : public templet::message {
public:
	range(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
    bool is_first;

    double left, right, fleft, fright, lrarea;
    double mid, fmid, larea, rarea;
    double area; bool area_computed;
};

struct range_task{
    range_task(double l,double r,double fl, double fr, double lr):left(l), right(r), fleft(fl), fright(fr), lrarea(lr){}
    double left, right, fleft, fright, lrarea;
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
        integral = 0.0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Master$engines*/
/*$TET$*/
	}

	inline void on_in(range&m) {
/*$TET$Master$in*/
		if (m.is_first) m.is_first = false;
		else {
/////// do it when the task is ready //////
            if(m.area_computed) integral += m.area;
            else{
                task_list.push_back(range_task(m.left,m.mid,m.fleft,m.fmid,m.larea));
                task_list.push_back(range_task(m.mid,m.right,m.fmid, m.fright,m.rarea));
            }
///////////////////////////////////////////            
		}
		range_list.push_back(&m);

		
		while (!range_list.empty() &&
//// check if we have something to do /////
            !task_list.empty()
///////////////////////////////////////////
			) {
			range* r = range_list.front();
			range_list.pop_front();
////////// form a new task ////////////////
            range_task t = task_list.front();
            task_list.pop_front();
            
            r->left = t.left; r->right = t.right;
            r->fleft = t.fleft; r->fright = t.fright;
            r->lrarea = t.lrarea; 
///////////////////////////////////////////
			r->send();
		}
		if (range_list.size() == N)	stop();        
/*$TET$*/
	}

	void in(range&m) { m.bind(this, &on_in_adapter); }

/*$TET$Master$$footer*/
    list<range*> range_list;
    list<range_task> task_list;
    double integral;
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
        out.is_first = true;
        out.send();
/*$TET$*/
	}

	inline void on_out(range&m) {
/*$TET$Worker$out*/
        //////// do the task  ////////
        m.mid = (m.left + m.right)/2;
        m.fmid = FUNCTION(m.mid);
        m.larea = (m.fleft + m.fmid)*(m.mid - m.left)/2.0;
        m.rarea = (m.fmid + m.fright)*(m.right - m.mid)/2.0;
        m.area = m.larea + m.rarea;
        m.area_computed = abs(m.area - m.lrarea) < epsilon;
        //////////////////////////////
        
        std::cout << "range[" << m.left << ";" << m.right << "]";
        if(m.area_computed) std::cout << " computed: " << m.area;
        else std::cout << " splited ";
        std::cout << " by worker # "<< ID << std::endl;
        
        out.send();
/*$TET$*/
	}

	range out;

/*$TET$Worker$$footer*/
    int ID;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
