/*--------------------------------------------------------------------------*/
/*  Copyright 2024 Sergei Vostokin                                          */
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

///////////// basic operations with prime numbers & sextuplets ///////////////

#pragma once
#include <list>

bool is_prime(long long num, std::list<long long>&table);
void extend_prime_table(std::list<long long>&table, long long to);
void find_sextuplets_in_range(std::list<long long>&table,long long from,long long to,std::list<long long>&found);
void sextuplets_search(std::list<long long>&table,long long to);