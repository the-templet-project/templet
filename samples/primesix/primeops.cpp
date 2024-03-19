#include "primeops.h"

#include <chrono>
#include <cmath>
#include <cassert>
#include <iostream>
#include <list>

using namespace std;

bool is_prime(long long num, std::list<long long>&table)
{
	for (long long divisor:table) {
		std::ldiv_t dv = std::ldiv(num, divisor);
		if (dv.rem == 0) return false;
		if (dv.quot <= divisor) break;
	}
	return true;
}

long long extend_prime_table(std::list<long long>&table,long long from, long long to)
{
    for(;from<=to;from+=2) if(is_prime(from,table))table.push_back(from);
    return from-2;
}

void find_sextuplets_in_range(std::list<long long>&table,long long from,long long to,std::list<long long>&found)
{
    int count = 0;
    
    long long n = from;
    long cur_size = 0;
    long long last_5[5]={0};
    found.clear();
    
	while (n <= to){
        if(is_prime(n, table)){
            if( 
                ++cur_size > 5 &&
                n==last_5[4]+4 &&
                n==last_5[3]+6 &&
                n==last_5[2]+10 &&
                n==last_5[1]+12 &&
                n==last_5[0]+16                 
            ){ 
                found.push_back(last_5[0]);
                //cout<<"sextuplet #" << ++count << " ("<<last_5[0]<<")" << endl;
            }
            
            last_5[0]=last_5[1];
            last_5[1]=last_5[2];
            last_5[2]=last_5[3];
            last_5[3]=last_5[4];
            last_5[4]=n;
        }
        n+=2;
    }    
}

void sextuplets_search(std::list<long long>&table,long long to)
{
    int  count = 0;
    long long n = 3;
    long cur_size = 0;
    long long last_5[5]={0};
    
	while (n <= to){
        if(is_prime(n, table)){
            table.push_back(n);
            if( 
                ++cur_size > 5 &&
                n==last_5[4]+4 &&
                n==last_5[3]+6 &&
                n==last_5[2]+10 &&
                n==last_5[1]+12 &&
                n==last_5[0]+16                 
            ){ 
                cout<<"sextuplet #" << ++count << " ("<<last_5[0]<<")" << endl;                
            }
            
            last_5[0]=last_5[1];
            last_5[1]=last_5[2];
            last_5[2]=last_5[3];
            last_5[3]=last_5[4];
            last_5[4]=n;
        }
        n+=2;
    }
}

