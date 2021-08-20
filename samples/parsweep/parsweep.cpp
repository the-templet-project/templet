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

#define _CRT_SECURE_NO_WARNINGS
#include "parsweep.hpp"

int main()
{
	app_ID = "everest-application-ID";
	maple_code = "../samples/parsweep/everest-app/code.txt";
	
	NUM_TASKS = 1;
	// set NUM_WORKERS in 'parsweep.hpp' if needed

	rho = new double[NUM_TASKS]   {28.0};
	sigma = new double[NUM_TASKS] {10.0};
	beta = new double[NUM_TASKS]  {2.66666};

	templet::engine eng;
	templet::everest_engine teng("everest-access-token");

	master mst(eng);
	worker wks[NUM_WORKERS];

	for (int i = 0; i < NUM_WORKERS; i++) {
		wks[i].engines(eng, teng);
		mst.r(wks[i].r);
	}

	eng.start();
	teng.run();
	
	if (!eng.stopped()) {
		templet::everest_error err;
		if (teng.error(&err)) templet::everest_engine::print_error(&err);
		else cout << "Logical error" << endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
