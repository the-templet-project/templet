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

/*$TET$*/

#pragma templet !Ping(out!message)

struct Ping :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Ping*)a)->on_out(*(message*)m);}

	Ping(templet::engine&e) :Ping() {
		Ping::engines(e);
	}

	Ping() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$Ping$Ping*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Ping$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Ping$start*/
        cout << "Ping started.." << endl;
        out.send();
/*$TET$*/
	}

	inline void on_out(message&m) {
/*$TET$Ping$out*/
        cout << "Ping received message from Pong.." << endl;
        stop();
/*$TET$*/
	}

	message out;

/*$TET$Ping$$footer*/
/*$TET$*/
};

#pragma templet Pong(in?message)

struct Pong :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Pong*)a)->on_in(*(message*)m);}

	Pong(templet::engine&e) :Pong() {
		Pong::engines(e);
	}

	Pong() :templet::actor(false)
	{
/*$TET$Pong$Pong*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Pong$engines*/
/*$TET$*/
	}

	inline void on_in(message&m) {
/*$TET$Pong$in*/
        cout << "Pong received message from Ping.." << endl;
        m.send();
/*$TET$*/
	}

	void in(message&m) { m.bind(this, &on_in_adapter); }

/*$TET$Pong$$footer*/
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
