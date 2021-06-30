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
           m.brick_ID = number_of_bricks--;
           m.send(); 
           
           std::cout << "the source worker passes a brick #" << m.brick_ID << std::endl;
        }
/*$TET$*/
	}

	brick out;

/*$TET$source$$footer*/
    int number_of_bricks;
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
        if(m.brick_ID == 1) stop(); else m.send();
        
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

	source       source_worker(eng);
	destination  destination_worker(eng);

    destination_worker.in(source_worker.out);
    
    source_worker.number_of_bricks = NUMBER_OF_BRICKS;
    
    eng.start();

	if (eng.stopped()) {
		std::cout << "all " << destination_worker.number_of_bricks 
            << " bricks were moved to a new place. done !!!";
		return EXIT_SUCCESS;
	}

	std::cout << "something broke (((" << std::endl;
	return EXIT_FAILURE;
}
/*$TET$*/
