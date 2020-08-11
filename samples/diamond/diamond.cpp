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

#include "diamond.hpp"
#include <iostream>

int main()
{
	engine eng;
	basesim_engine teng;

	//     A(1)     |
	//    /    \    |
	//  B(2)  C(3)  |
	//    \    /    |
	//     D(4)     V

	A a(eng,teng);
	B b(eng,teng);
	C c(eng,teng);
	D d(eng,teng);

	a.delay = 1.0;
	b.delay = 2.0;
	c.delay = 3.0;
	d.delay = 4.0;

	b.a(a.b); c.a(a.c);
	d.b(b.d); d.c(c.d);

	eng.start();
	teng.run();

	if (eng.stopped()) {
		std::cout << "Success!!!" << std::endl;
		
		std::cout << "Maximum number of tasks executed in parallel : " << teng.Pmax() << std::endl;
		std::cout << "Time of sequential execution of all tasks    : " << teng.T1() << std::endl;
		std::cout << "Time of parallel   execution of all tasks    : " << teng.Tp() << std::endl;
		
		return EXIT_SUCCESS;
	}

	std::cout << "Failure..." << std::endl;
	return EXIT_FAILURE;
}