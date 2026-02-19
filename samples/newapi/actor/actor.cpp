#include "diamond.hpp"

#include <iostream>
#include <thread>
#include <atomic>

const int NUM_THREADS = 10;

int main()
{
	std::atomic_int PID = 0;

	templet::write_ahead_log wal;
	std::vector<std::thread> threads(NUM_THREADS);

	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	engine eng;
	syncmem_engine teng(wal);

	//     A     |
	//    / \    |
	//   B   C   |
	//    \ /    |
	//     D     V

	A a(eng, teng);
	B b(eng, teng);
	C c(eng, teng);
	D d(eng, teng);

	b.a(a.b); c.a(a.c);
	d.b(b.d); d.c(c.d);

	eng.start();
	teng.run();

	if (pid == 0) {// .. or any other pid < NUM_THREADS
		if (eng.stopped()) {
			std::cout << "Success!!!" << std::endl;

			std::cout << a.str << std::endl;
			std::cout << b.str << std::endl;
			std::cout << c.str << std::endl;
			std::cout << d.str << std::endl;
		}
		else
			std::cout << "Failure..." << std::endl;
	}

	////////////////////////////////////////////////////
	}); for (auto& t : threads) t.join();
	std::cout << "Completed..." << std::endl;
}