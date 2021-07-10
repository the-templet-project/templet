/*$TET$$header*/
/*--------------------------------------------------------------------------*/
/*  Copyright 2021 Sergei Vostokin                                          */
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

const int NUMBER_OF_MEDIATORS = 3;
const int NUMBER_OF_BRICKS = 2;

#include <templet.hpp>
#include <cmath>
#include <iostream>
#include <ctime>

class brick : public templet::message {
public:
	brick(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
	int brick_ID;
};
/*$TET$*/

#pragma templet !source(out!brick)

struct source :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((source*)a)->on_out(*(brick*)m);}

	source(templet::engine&e) :source() {
		source::engines(e);
	}

	source() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$source$source*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$source$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$source$start*/
        on_out(out);
/*$TET$*/
	}

	inline void on_out(brick&m) {
/*$TET$source$out*/
       if(number_of_bricks > 0){ 
           out.brick_ID = number_of_bricks--;
           out.send(); 
           
		   std::cout << "the source worker passes a brick #" << m.brick_ID << std::endl;
        }
/*$TET$*/
	}

	brick out;

/*$TET$source$$footer*/
    int number_of_bricks;
/*$TET$*/
};

#pragma templet mediator(in?brick,out!brick,t:base)

struct mediator :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((mediator*)a)->on_in(*(brick*)m);}
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((mediator*)a)->on_out(*(brick*)m);}
	static void on_t_adapter(templet::actor*a, templet::task*t) {
		((mediator*)a)->on_t(*(templet::base_task*)t);}

	mediator(templet::engine&e,templet::base_engine&te_base) :mediator() {
		mediator::engines(e,te_base);
	}

	mediator() :templet::actor(false),
		out(this, &on_out_adapter),
		t(this, &on_t_adapter)
	{
/*$TET$mediator$mediator*/
		_in = 0;
/*$TET$*/
	}

	void engines(templet::engine&e,templet::base_engine&te_base) {
		templet::actor::engine(e);
		t.engine(te_base);
/*$TET$mediator$engines*/
/*$TET$*/
	}

	inline void on_in(brick&m) {
/*$TET$mediator$in*/
		_in = &m;
		take_a_brick();
/*$TET$*/
	}

	inline void on_out(brick&m) {
/*$TET$mediator$out*/
		take_a_brick();
/*$TET$*/
	}

	inline void on_t(templet::base_task&t) {
/*$TET$mediator$t*/
		pass_a_brick();
/*$TET$*/
	}

	void in(brick&m) { m.bind(this, &on_in_adapter); }
	brick out;
	templet::base_task t;

/*$TET$mediator$$footer*/
	void take_a_brick() {
		if (access(_in) && access(out)) {

			brick_ID = _in->brick_ID;
			_in->send();
			std::cout << "the mediator worker #"
				<< mediator_ID << " takes a brick #" << brick_ID << std::endl;
			
			t.submit();
		}
	}

	void pass_a_brick() {
		out.brick_ID = brick_ID;
		out.send();
		std::cout << "the mediator worker #"
			<< mediator_ID << " passes a brick #" << brick_ID << std::endl;
	}

	brick* _in;
	int brick_ID;
	int mediator_ID;
/*$TET$*/
};

#pragma templet destination(in?brick)

struct destination :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((destination*)a)->on_in(*(brick*)m);}

	destination(templet::engine&e) :destination() {
		destination::engines(e);
	}

	destination() :templet::actor(false)
	{
/*$TET$destination$destination*/
        number_of_bricks = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$destination$engines*/
/*$TET$*/
	}

	inline void on_in(brick&m) {
/*$TET$destination$in*/
        number_of_bricks++;
        if(m.brick_ID == 1) stop(); else  m.send();
        
		std::cout << "the destination worker takes a brick #" << m.brick_ID << std::endl;
/*$TET$*/
	}

	void in(brick&m) { m.bind(this, &on_in_adapter); }

/*$TET$destination$$footer*/
    int number_of_bricks;
/*$TET$*/
};

/*$TET$$footer*/

int main()
{
	templet::engine eng;
    templet::base_engine teng;
    
	source       source_worker(eng);
	mediator     mediator_worker[NUMBER_OF_MEDIATORS];
	destination  destination_worker(eng);

    mediator_worker[0].in(source_worker.out);
    
    for(int i=1; i<NUMBER_OF_MEDIATORS; i++ )       
        mediator_worker[i].in(mediator_worker[i-1].out);

    destination_worker.in(mediator_worker[NUMBER_OF_MEDIATORS-1].out);
    
    source_worker.number_of_bricks = NUMBER_OF_BRICKS;
    
    for(int i=0;i<NUMBER_OF_MEDIATORS;i++){
        mediator_worker[i].mediator_ID = i+1;
        mediator_worker[i].engines(eng,teng);
    }
    
	srand(time(NULL));

    eng.start();
    teng.run();

	if (eng.stopped()) {
		std::cout << "all " << destination_worker.number_of_bricks 
            << " bricks were moved to a new place. done !!!";
		return EXIT_SUCCESS;
	}

	std::cout << "something broke (((" << std::endl;
	return EXIT_FAILURE;
}
/*$TET$*/
