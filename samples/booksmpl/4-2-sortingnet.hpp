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

class num : public templet::message {
public:
	num(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
    int element_number;
};
/*$TET$*/

#pragma templet !Start(out!num)

struct Start :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Start*)a)->on_out(*(num*)m);}

	Start(templet::engine&e) :Start() {
		Start::engines(e);
	}

	Start() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$Start$Start*/
        out.send();
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Start$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Start$start*/
/*$TET$*/
	}

	inline void on_out(num&m) {
/*$TET$Start$out*/
/*$TET$*/
	}

	num out;

/*$TET$Start$$footer*/
/*$TET$*/
};

#pragma templet Stop(in?num)

struct Stop :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Stop*)a)->on_in(*(num*)m);}

	Stop(templet::engine&e) :Stop() {
		Stop::engines(e);
	}

	Stop() :templet::actor(false)
	{
/*$TET$Stop$Stop*/
        num_of_computed_elements = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Stop$engines*/
/*$TET$*/
	}

	inline void on_in(num&m) {
/*$TET$Stop$in*/
        if(++num_of_computed_elements == N) stop();
/*$TET$*/
	}

	void in(num&m) { m.bind(this, &on_in_adapter); }

/*$TET$Stop$$footer*/
    int num_of_computed_elements;
/*$TET$*/
};

#pragma templet Pair(in1?num,in2?num,out1!num,out2!num)

struct Pair :public templet::actor {
	static void on_in1_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_in1(*(num*)m);}
	static void on_in2_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_in2(*(num*)m);}
	static void on_out1_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_out1(*(num*)m);}
	static void on_out2_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_out2(*(num*)m);}

	Pair(templet::engine&e) :Pair() {
		Pair::engines(e);
	}

	Pair() :templet::actor(false),
		out1(this, &on_out1_adapter),
		out2(this, &on_out2_adapter)
	{
/*$TET$Pair$Pair*/
        _in1 = _in2 = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Pair$engines*/
/*$TET$*/
	}

	inline void on_in1(num&m) {
/*$TET$Pair$in1*/
        _in1 = &m;  swap();
/*$TET$*/
	}

	inline void on_in2(num&m) {
/*$TET$Pair$in2*/
        _in2 = &m;  swap();
/*$TET$*/
	}

	inline void on_out1(num&m) {
/*$TET$Pair$out1*/
/*$TET$*/
	}

	inline void on_out2(num&m) {
/*$TET$Pair$out2*/
/*$TET$*/
	}

	void in1(num&m) { m.bind(this, &on_in1_adapter); }
	void in2(num&m) { m.bind(this, &on_in2_adapter); }
	num out1;
	num out2;

/*$TET$Pair$$footer*/
    void swap(){
        if(access(_in1) && access(_in2)){
            if(arr[_in1->element_number] > arr[_in2->element_number]){
                int tmp = arr[_in2->element_number];
                arr[_in2->element_number] = arr[_in1->element_number];
                arr[_in1->element_number] = tmp;
            }
            out1.element_number = _in1->element_number;
            out2.element_number = _in2->element_number;
            
            out1.send();out2.send();
        }
    }
    
    num* _in1;
    num* _in2;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
