#include "diamond.hpp"
#include <iostream>

int main()
{
	write_ahead_log log;
	engine eng;
	syncmem_engine teng(log);

	//     A(1)     |
	//    /    \    |
	//  B(2)  C(3)  |
	//    \    /    |
	//     D(4)     V

	A a(eng, teng);
	B b(eng, teng);
	C c(eng, teng);
	D d(eng, teng);

	b.a(a.b); c.a(a.c);
	d.b(b.d); d.c(c.d);

	eng.start();
	teng.run();

	if (eng.stopped()) {
		std::cout << "Success!!!" << std::endl;

		std::cout << a.str << std::endl;
		std::cout << b.str << std::endl;
		std::cout << c.str << std::endl;
		std::cout << d.str << std::endl;

		return EXIT_SUCCESS;
	}

	std::cout << "Failure..." << std::endl;
	return EXIT_FAILURE;
}