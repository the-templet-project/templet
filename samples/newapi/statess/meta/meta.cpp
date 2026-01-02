#include <iostream>
#include <fstream>
#include <syncmem.hpp>

class meta_scanner : public templet::meta::state {
public:
	meta_scanner() {
		prefix("template <typename T>"); name("scanner");

		def("set_ready_to_compute", action::_update);

		def("share_element", action::_save_update)
			.par("unsigned index", "0")
			.par("T&element", "element", "T element");

		def("bool", "get_not_scanned", action::_output)
			.par("unsigned& index", "index", "unsigned index")
			.par("int random", "0");

		def("put_scanned", action::_save_update)
			.par("unsigned index", "0")
			.par("T&element", "element", "T element");
	}
};

int main()
{
	meta_scanner meta_scanner_object;
	std::ofstream file("statess.cpp.txt");
	meta_scanner_object.print(file);
}

