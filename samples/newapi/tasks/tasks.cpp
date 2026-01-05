#include <iostream>
#include <thread>
#include <atomic>

//#define TEMPLET_TASK_TEST_IMPL
#include <syncmem.hpp>

const int NUM_THREADS = 10;
const int ARRAY_SIZE = 10;

int main()
{
	std::atomic_int PID = 0;

	templet::write_ahead_log wal;
	std::vector<std::thread> threads(NUM_THREADS);

	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	int N[ARRAY_SIZE]{0};
	int NxN[ARRAY_SIZE]{0};

	templet::task_engine eng(wal);

	for (int i = 0; i < ARRAY_SIZE; i++) {
		eng.async(pid == 0,
			[i](std::ostream&out) {
			out << i;
		},
			[i,&N](std::istream&in) {
			in >> N[i];
		}
		);
	}
	eng.await();

	for (int i = 0; i < ARRAY_SIZE; i++) {
		eng.async(
			[&N,i](std::ostream&out) {
			out << N[i] * N[i];
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
