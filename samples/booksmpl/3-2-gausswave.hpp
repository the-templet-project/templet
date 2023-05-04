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

class border : public templet::message {
public:
	border(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
};
/*$TET$*/

#pragma templet !Begin(out!border)

struct Begin :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Begin*)a)->on_out(*(border*)m);}

	Begin(templet::engine&e) :Begin() {
		Begin::engines(e);
	}

	Begin() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$Begin$Begin*/
        wave_num = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Begin$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Begin$start*/
        on_out(out);
/*$TET$*/
	}

	inline void on_out(border&m) {
/*$TET$Begin$out*/
        if(++wave_num <= T) out.send();
/*$TET$*/
	}

	border out;

/*$TET$Begin$$footer*/
    int wave_num;
/*$TET$*/
};

#pragma templet Stage(in?border,out!border)

struct Stage :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Stage*)a)->on_in(*(border*)m);}
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Stage*)a)->on_out(*(border*)m);}

	Stage(templet::engine&e) :Stage() {
		Stage::engines(e);
	}

	Stage() :templet::actor(false),
		out(this, &on_out_adapter)
	{
/*$TET$Stage$Stage*/
        _in = 0; 
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Stage$engines*/
/*$TET$*/
	}

	inline void on_in(border&m) {
/*$TET$Stage$in*/
        _in = &m;
        calculate();
/*$TET$*/
	}

	inline void on_out(border&m) {
/*$TET$Stage$out*/
        calculate();
/*$TET$*/
	}

	void in(border&m) { m.bind(this, &on_in_adapter); }
	border out;

/*$TET$Stage$$footer*/
    void calculate(){
        if(access(_in) && access(out)){
            area[offset] = 0.5*(area[offset-1]+area[offset+1]);
            _in->send(); out.send();
        }        
    }
    border* _in;
    int  offset;
/*$TET$*/
};

#pragma templet End(in?border)

struct End :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((End*)a)->on_in(*(border*)m);}

	End(templet::engine&e) :End() {
		End::engines(e);
	}

	End() :templet::actor(false)
	{
/*$TET$End$End*/
        wave_num = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$End$engines*/
/*$TET$*/
	}

	inline void on_in(border&m) {
/*$TET$End$in*/
        if(++wave_num == T) stop();
        else m.send();
/*$TET$*/
	}

	void in(border&m) { m.bind(this, &on_in_adapter); }

/*$TET$End$$footer*/
    int wave_num;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
