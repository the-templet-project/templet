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

#define _CRT_SECURE_NO_DEPRECATE

#include <templet.hpp>
#include <everest.hpp>
#include <string>
#include <iostream>

class hello : public templet::message {
public:
	hello(templet::actor*a, templet::message_adaptor ma) :templet::message(a, ma) {}
	std::string str;
};

/*$TET$*/

#pragma templet !ping(p!hello)

struct ping :public templet::actor {
	static void on_p_adapter(templet::actor*a, templet::message*m) {
		((ping*)a)->on_p(*(hello*)m);}

	ping(templet::engine&e) :ping() {
		ping::engines(e);
	}

	ping() :templet::actor(true),
		p(this, &on_p_adapter)
	{
/*$TET$ping$ping*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$ping$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$ping$start*/
		p.send();
/*$TET$*/
	}

	inline void on_p(hello&m) {
/*$TET$ping$p*/
		stop();
/*$TET$*/
	}

	hello p;

/*$TET$ping$$footer*/
/*$TET$*/
};

#pragma templet pong(p?hello,t:everest)

struct pong :public templet::actor {
	static void on_p_adapter(templet::actor*a, templet::message*m) {
		((pong*)a)->on_p(*(hello*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((pong*)a)->on_t(*(templet::everest_task*)t);}

	pong(templet::engine&e,templet::everest_engine&te_everest) :pong() {
		pong::engines(e,te_everest);
	}

	pong() :templet::actor(false),
		t(this, &on_t_adapter)
	{
/*$TET$pong$pong*/
		_p = 0;
		t.app_id("Everest-application-ID");
/*$TET$*/
	}

	void engines(templet::engine&e,templet::everest_engine&te_everest) {
		templet::actor::engine(e);
		t.engine(te_everest);
/*$TET$pong$engines*/
/*$TET$*/
	}

	inline void on_p(hello&m) {
/*$TET$pong$p*/
		_p = &m;

		json in;
		in["name"] = "echo-application";
		in["inputs"]["input-string"] = m.str;

		if (t.submit(in)) std::cout << "task submit succeeded" << std::endl;
		else std::cout << "task submit failed" << std::endl;
/*$TET$*/
	}

	inline void on_t(templet::everest_task&t) {
/*$TET$pong$t*/
		json out = t.result();
		_p->str =  out["output-string"];
		_p->send();
/*$TET$*/
	}

	void p(hello&m) { m.bind(this, &on_p_adapter); }
	templet::everest_task t;

/*$TET$pong$$footer*/
	hello* _p;
/*$TET$*/
};

/*$TET$$footer*/
int main()
{
	templet::engine eng;
	templet::everest_engine teng("access-token");
	
	//--- for getting access-token and Everest-application-ID use the following code ---
	//templet::everest_engine teng("your-Everest-login","your-password");
	//teng.save_access_token();

	//if (teng) {
	//	teng.print_app_description("application-name");
	//	string token;
	//	teng.get_access_token(token);
	//	std::cout << "\n" << token << "\n";
	//  return EXIT_SUCCESS;
	//}

	//--- How to setup the new Everest echo application ---
	// 1) Enter Name and Alias on the Metadata tab.
	// 2) Enter input-string:string and output-string:string on the Parameters tab.
	// 3) For Windows: enter the Command: 'cmd.exe /c echo ECHO:${input-string} > out.txt',
	//    when the Output Mappings: output-string --> out.txt on the Configuration tab.
	// 4) Select a resource to run your application (it should be Windows machine).
	//--- See the page http://everest.distcomp.org/docs/tutorial/#_adding_applications for details. ---


	if (!teng) {
		std::cout << "task engine is not connected to the Everest server..." << std::endl;
		return EXIT_FAILURE;
	}

	ping a_ping(eng);
	pong a_pong(eng, teng);

	a_pong.p(a_ping.p);

	a_ping.p.str = "Yoo-Hoo!";

	eng.start();

try_continue:
	teng.run();

	if (eng.stopped()) {
		std::cout << "answer from remote Pong:" << std::endl;
		std::cout << a_ping.p.str << std::endl;

		return EXIT_SUCCESS;
	}

	static int recovery_tries = 3;
	templet::everest_error cntxt;

	if (teng.error(&cntxt)) {
		std::cout << "...task engine error..." << std::endl;

		if (recovery_tries--) {
			std::cout << "error information:" << std::endl;

			std::cout << "type ID : " << *cntxt._type << std::endl;
			std::cout << "HTML response code : " << cntxt._code << std::endl;
			std::cout << "HTML response : " << cntxt._response << std::endl;
			std::cout << "task input : " << cntxt._task_input << std::endl;

			// trying to fix an error
			switch (*cntxt._type) {
				case templet::everest_error::NOT_CONNECTED:
				{
					std::cout << "error string : NOT_CONNECTED" << std::endl;
					std::cout << "the task engine is not initialized properly -- fatal error, exiting" << std::endl;
					return EXIT_FAILURE;
				}
				case templet::everest_error::SUBMIT_FAILED:
				{
					std::cout << "error string : SUBMIT_FAILED" << std::endl;
					std::cout << "resubmitting the task" << std::endl;
					json input = json::parse(cntxt._task_input);
					// here you may fix something in the input 
					cntxt._task->resubmit(input);
					break;
				}
				case templet::everest_error::TASK_CHECK_FAILED:
				{
					std::cout << "error string : TASK_CHECK_FAILED" << std::endl;
					std::cout << "trying to check the task status again" << std::endl;
					*cntxt._type = templet::everest_error::NOT_ERROR;
					break;
				}
				case templet::everest_error::TASK_FAILED_OR_CANCELLED:
				{
					std::cout << "error string : TASK_FAILED_OR_CANCELLED" << std::endl;
					std::cout << "resubmitting the task" << std::endl;
					json input = json::parse(cntxt._task_input);
					// here you may fix something in the input 
					cntxt._task->resubmit(input);
				}
			}
	
			goto try_continue;
		}
	}
	else 
		std::cout << "logical error" << std::endl;
	
	return EXIT_FAILURE;
}
/*$TET$*/
