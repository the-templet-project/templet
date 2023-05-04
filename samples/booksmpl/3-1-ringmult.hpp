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

class column : public templet::message {
public:
	column(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
    int j; // column index
};

/*$TET$*/

#pragma templet !Link(in?column,out!column,stop!message)

struct Link :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Link*)a)->on_in(*(column*)m);}
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Link*)a)->on_out(*(column*)m);}
	static void on_stop_adapter(templet::actor*a, templet::message*m) {
		((Link*)a)->on_stop(*(message*)m);}

	Link(templet::engine&e) :Link() {
		Link::engines(e);
	}

	Link() :templet::actor(true),
		out(this, &on_out_adapter),
		stop(this, &on_stop_adapter)
	{
/*$TET$Link$Link*/
        num_of_ready_columns = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Link$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Link$start*/
        on_in(out);
/*$TET$*/
	}

	inline void on_in(column&m) {
/*$TET$Link$in*/
        if(num_of_ready_columns!=0 &&  &m == &out) return;
        
        int j = m.j;
        for(int k = 0; k < N; k++) C[i][j] += A[i][k] * B[k][j];
        
        next->in(m); m.send();
        if(++num_of_ready_columns == N) stop.send();       
/*$TET$*/
	}

	inline void on_out(column&m) {
/*$TET$Link$out*/
/*$TET$*/
	}

	inline void on_stop(message&m) {
/*$TET$Link$stop*/
/*$TET$*/
	}

	void in(column&m) { m.bind(this, &on_in_adapter); }
	column out;
	message stop;

/*$TET$Link$$footer*/ 
    int i; // row index
    int num_of_ready_columns;
    Link *next;
/*$TET$*/
};

#pragma templet !Stopper(in?message)

struct Stopper :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Stopper*)a)->on_in(*(message*)m);}

	Stopper(templet::engine&e) :Stopper() {
		Stopper::engines(e);
	}

	Stopper() :templet::actor(true)
	{
/*$TET$Stopper$Stopper*/
        num_of_ready_rows = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Stopper$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Stopper$start*/
/*$TET$*/
	}

	inline void on_in(message&m) {
/*$TET$Stopper$in*/
        if(++num_of_ready_rows == N) stop();
/*$TET$*/
	}

	void in(message&m) { m.bind(this, &on_in_adapter); }

/*$TET$Stopper$$footer*/
    int num_of_ready_rows;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
