#include <iostream>
#include <thread>
#include <atomic>

//#define TEMPLET_TASK_TEST_IMPL
#include <syncmem.hpp>
#include <walimpl.hpp>

const int NUM_THREADS = 2;
const int ARRAY_SIZE = 20;

int main()
{
	std::atomic_int PID = 0;

	//templet::write_ahead_log wal;
    templet::server_side_wal server_wal(1000,1,std::string("file"), std::string("txt"));
    templet::client_side_wal wal(1000,1,std::string("file"), std::string("txt"),server_wal);
	
    std::vector<std::thread> threads(NUM_THREADS);

	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	int N[ARRAY_SIZE]{0};
	int NxN[ARRAY_SIZE]{0};

	templet::task_engine eng(wal);

    std::srand((unsigned)time(NULL));
                                                 
	for (int i = 0; i < ARRAY_SIZE; i++) {
		eng.async(pid == 0,
			[i,pid](std::ostream&out) {
			out << i;              std::cout << " (for1 pid=" << pid << " i=" << i <<")" << std::endl;
            std::this_thread::sleep_for(1s);
		},
			[i,&N](std::istream&in) {
			in >> N[i];
		}
		);
	}
	eng.await();

	for (int i = 0; i < ARRAY_SIZE; i++) {
		eng.async(
			[&N,i,pid](std::ostream&out) {
			out << N[i] * N[i];  std::cout << " (for2 pid=" << pid << " i=" << i <<")" << std::endl;
            std::this_thread::sleep_for(1s);
    	},
			[&NxN,i](std::istream&in) {
			in >> NxN[i];
        }
        );
    }
	eng.await();

	if (pid == 0) {// .. or any other pid < NUM_THREADS 
		for (int i = 0; i < ARRAY_SIZE; i++)
			std::cout << i << " * " << i << " = "
			<< NxN[i] << std::endl;
	}
	////////////////////////////////////////////////////
	}); for (auto& t : threads) t.join();
	std::cout << "Success!" << std::endl;
}
