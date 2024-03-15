#include "primeops.h"

#include <chrono>
#include <cmath>
#include <cassert>
#include <iostream>
#include <list>

using namespace std;

const long long SEARCH_RANGE_LIMIT = 100000000;

int main()
{
    list<long long> table;

    // 1 case
    auto start = std::chrono::high_resolution_clock::now();
	sextuplets_search(table,SEARCH_RANGE_LIMIT);
	auto end = std::chrono::high_resolution_clock::now();
    
	std::chrono::duration<double> diff = end - start;	
    cout << "sequential exec. time = " << diff.count() << "s" << endl;

    // 2 case
    table.clear();
    std::list<long long> found;

    start = std::chrono::high_resolution_clock::now();
    
    long long max_num_in_table = (long long)sqrt(SEARCH_RANGE_LIMIT);

    cout << "max_num_in_table = " << max_num_in_table << endl;
    
    extend_prime_table(table, 3, max_num_in_table);
    
    find_sextuplets_in_range(table,3,SEARCH_RANGE_LIMIT,found);
 
    end = std::chrono::high_resolution_clock::now();
    
    diff = end - start;	
    cout << "sequential exec. time = " << diff.count() << "s" << endl;

    return EXIT_SUCCESS;
}
