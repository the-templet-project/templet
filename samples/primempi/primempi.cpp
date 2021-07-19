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

#include <chrono>
#include <cmath>
#include <iostream>
#include <ctime>

bool update_prime_table(long num, long*table, int*cur_size, int max_size)
{
	for (int i = 0; i < *cur_size; i++) {
		long divisor = table[i];
		std::ldiv_t dv = std::div(num, divisor);
		if (dv.rem == 0) return true;
		if (dv.quot <= divisor) break;
	}

	if (*cur_size < max_size) {
		table[(*cur_size)++] = num;
		return true;
	}

	return false;
}

bool maybe_prime_number(long num, long*table, int max_size)
{
	for (int i = 0; i < max_size; i++) {
		long divisor = table[i];
		std::ldiv_t dv = std::div(num, divisor);
		if (dv.rem == 0) return false;
		if (dv.quot <= divisor) break;
	}

	return true;
}

void build_prime_table(long*table, int size)
{
	long n = 3; int cur_size = 0;
	while (update_prime_table(n, table, &cur_size, size))n += 2;
}

const int size = 10000000;
long table[size];
/*$TET$*/

/*$TET$$footer*/
int main()
{
	

	auto start = std::chrono::high_resolution_clock::now();
	build_prime_table(table, size);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end - start;

	std::cout << "duration = " << diff.count() << " sec";

	return EXIT_SUCCESS;
}
/*$TET$*/
