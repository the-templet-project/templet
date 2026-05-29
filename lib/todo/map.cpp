#include "wal.hpp"
#include "map.hpp"
#include "extra.hpp"

#include <iostream>

const int NUM_PROC = 10;
const int SIZE = 10;    

int main()
{
    templet::wal wal;
    templet::job job(NUM_PROC);

    job([&](unsigned pid){

        templet::map map(wal);

        std::vector<int> N;
        std::vector<int> NxN;

        if(pid==0){// in master 'process'
            N.resize(SIZE); NxN.resize(SIZE);
            for(int i=0; i<SIZE; i++) N[i]=i+1;
            map.init(SIZE);
        }

        map(
            [&](unsigned size){ N.resize(size); NxN.resize(size); },
            [&](unsigned iter){ 
                NxN[iter] = N[iter]*N[iter];
                templet::job::delay(1.0);//simulate workload
            },
            [&](unsigned iter, std::ostream& out, bool mapped) {
                if(!mapped) out << N[iter];
                else out << NxN[iter];
            },
            [&](unsigned iter, std::istream& in, bool mapped) {
                if(!mapped) in >> N[iter];
                else in >> NxN[iter];
            }
        );
    
        if(pid==0){// in master 'process'
            for(int i=0;i<SIZE;i++)
                std::cout << N[i] <<"^2 = " << NxN[i] << std::endl;
        }             
    });

    std::cout << "Duration with " << NUM_PROC << 
        " thread(s) is " << job.duration() << " seconds." << std::endl;
}
