#include <iostream>
#include <fstream>
#include <string>

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
	std::string file = "..\\statess.cpp"; // file to process
	std::string skel = "C:\\Users\\Сергей\\Documents\\GitHub\\templet\\bin\\skel.exe";// path to 'templet\bin\skel.exe'

	if (system(NULL) == 0) {
		std::cout << "ERROR: command processor is not detected";
		return EXIT_FAILURE;
	};

	std::string command;
	int ret;

	{
		meta_scanner meta_scanner_object;
		std::ofstream gen(file + ".cgn");
		meta_scanner_object.print(gen);
	}

#if defined(_WIN32) || defined(_WIN64) 
	command = "copy " + file + " " + file + ".bak";
#else
	command = "cp " + file + " " + file + ".bak";
#endif
	ret = system(command.c_str());

	if (ret == EXIT_FAILURE) {
		std::cout << "ERROR: failed to backup " << file << " to " << (file + ".bak");
		return EXIT_FAILURE;
	}

	command = skel + " -i " + file + " -s " + (file + ".cgn");
	ret = system(command.c_str());

	if (ret == EXIT_FAILURE) {
#if defined(_WIN32) || defined(_WIN64)
		command = "copy " + (file + ".bak") + " " + file;
#else	
		command = "cp " + (file + ".bak") + " " + file;
#endif
		system(command.c_str());
	}

#if defined(_WIN32) || defined(_WIN64)
	command = "del " + (file + ".cgn");
#else
	command = "rm " + (file + ".cgn");
#endif
	system(command.c_str());

	if (ret == EXIT_SUCCESS) std::cout << "Ok";

	return ret;
}

