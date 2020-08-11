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

#pragma once
#include <xtemplet.hpp>
#include <xbasesim.hpp>
using namespace templet;
/*$TET$*/

#pragma templet !A(b!message,c!message,t:basesim)

struct A :public templet::actor {
	static void on_b_adapter(templet::actor*a, templet::message*m) {
		((A*)a)->on_b(*(message*)m);}
	static void on_c_adapter(templet::actor*a, templet::message*m) {
		((A*)a)->on_c(*(message*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((A*)a)->on_t(*(templet::basesim_task*)t);}

	A(templet::engine&e,templet::basesim_engine&te_basesim) :A() {
		A::engines(e,te_basesim);
	}

	A() :templet::actor(true),
		b(this, &on_b_adapter),
		c(this, &on_c_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$A$A*/
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$A$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$A$start*/
		t.submit();
/*$TET$*/
	}

	inline void on_b(message&m) {
/*$TET$A$b*/
/*$TET$*/
	}

	inline void on_c(message&m) {
/*$TET$A$c*/
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$A$t*/
		t.delay(delay);
		b.send(); c.send();
/*$TET$*/
	}

	message b;
	message c;
	templet::basesim_task t;

/*$TET$A$$footer*/
	double delay;
/*$TET$*/
};

#pragma templet B(a?message,d!message,t:basesim)

struct B :public templet::actor {
	static void on_a_adapter(templet::actor*a, templet::message*m) {
		((B*)a)->on_a(*(message*)m);}
	static void on_d_adapter(templet::actor*a, templet::message*m) {
		((B*)a)->on_d(*(message*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((B*)a)->on_t(*(templet::basesim_task*)t);}

	B(templet::engine&e,templet::basesim_engine&te_basesim) :B() {
		B::engines(e,te_basesim);
	}

	B() :templet::actor(false),
		d(this, &on_d_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$B$B*/
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$B$engines*/
/*$TET$*/
	}

	inline void on_a(message&m) {
/*$TET$B$a*/
		t.submit();
/*$TET$*/
	}

	inline void on_d(message&m) {
/*$TET$B$d*/
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$B$t*/
		t.delay(delay);
		d.send();
/*$TET$*/
	}

	void a(message&m) { m.bind(this, &on_a_adapter); }
	message d;
	templet::basesim_task t;

/*$TET$B$$footer*/
	double delay;
/*$TET$*/
};

#pragma templet C(a?message,d!message,t:basesim)

struct C :public templet::actor {
	static void on_a_adapter(templet::actor*a, templet::message*m) {
		((C*)a)->on_a(*(message*)m);}
	static void on_d_adapter(templet::actor*a, templet::message*m) {
		((C*)a)->on_d(*(message*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((C*)a)->on_t(*(templet::basesim_task*)t);}

	C(templet::engine&e,templet::basesim_engine&te_basesim) :C() {
		C::engines(e,te_basesim);
	}

	C() :templet::actor(false),
		d(this, &on_d_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$C$C*/
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$C$engines*/
/*$TET$*/
	}

	inline void on_a(message&m) {
/*$TET$C$a*/
		t.submit();
/*$TET$*/
	}

	inline void on_d(message&m) {
/*$TET$C$d*/
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$C$t*/
		t.delay(delay);
		d.send();
/*$TET$*/
	}

	void a(message&m) { m.bind(this, &on_a_adapter); }
	message d;
	templet::basesim_task t;

/*$TET$C$$footer*/
	double delay;
/*$TET$*/
};

#pragma templet D(b?message,c?message,t:basesim)

struct D :public templet::actor {
	static void on_b_adapter(templet::actor*a, templet::message*m) {
		((D*)a)->on_b(*(message*)m);}
	static void on_c_adapter(templet::actor*a, templet::message*m) {
		((D*)a)->on_c(*(message*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((D*)a)->on_t(*(templet::basesim_task*)t);}

	D(templet::engine&e,templet::basesim_engine&te_basesim) :D() {
		D::engines(e,te_basesim);
	}

	D() :templet::actor(false),
		t(this, &on_t_adapter)
	{
/*$TET$D$D*/
		_b = _c = 0;
/*$TET$*/
	}

	void engines(templet::engine&e,templet::basesim_engine&te_basesim) {
		templet::actor::engine(e);
		t.engine(te_basesim);
/*$TET$D$engines*/
/*$TET$*/
	}

	inline void on_b(message&m) {
/*$TET$D$b*/
		_b = &m; on_b_and_c();
/*$TET$*/
	}

	inline void on_c(message&m) {
/*$TET$D$c*/
		_c = &m; on_b_and_c();
/*$TET$*/
	}

	inline void on_t(templet::basesim_task&t) {
/*$TET$D$t*/
		t.delay(delay);
		stop();
/*$TET$*/
	}

	void b(message&m) { m.bind(this, &on_b_adapter); }
	void c(message&m) { m.bind(this, &on_c_adapter); }
	templet::basesim_task t;

/*$TET$D$$footer*/
	message *_c;
	message *_b;
	double delay;
	void on_b_and_c(){ if(access(_c) && access(_b)) t.submit(); }
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
