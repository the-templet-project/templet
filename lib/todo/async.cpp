#include "wal.hpp"
#include "async.hpp"
#include "extra.hpp"

#include <iostream>

const int NUM_PROC = 10;
const int SIZE = 10;

int main()
{  
    templet::wal wal;
    templet::job job(NUM_PROC);

    job([&](unsigned pid){

        templet::async async(wal);

        unsigned size;
        std::vector<int> N;
        std::vector<int> NxN;
        
        async.task( pid==0,
            [&](std::ostream& out){ 
                out << SIZE << " ";
                for(int i=0; i<SIZE; i++) out << (i+1) << " ";
            },
			[&](std::istream& in){ 
                in >> size; 
                N.resize(size); NxN.resize(size);
                for(int i=0; i<size; i++) in >> N[i];
            }
        );

        async.wait();

        for(int i=0; i<size; i++) async.task(
            [&](std::ostream& out){ 
                NxN[i] = N[i]*N[i];
                templet::job::delay(1.0);//simulate workload
                out << NxN[i];
            },
            [&](std::istream& in){ 
                in >> NxN[i];
            }
        );
        
        async.wait();
            
        if(pid==0){// in master 'process'
            for(int i=0;i<SIZE;i++)
                std::cout << N[i] <<"^2 = " << NxN[i] << std::endl;
        }             
    });

    std::cout << "Duration with " << NUM_PROC << 
        " thread(s) is " << job.duration() << " seconds." << std::endl;
}
