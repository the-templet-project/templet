#include "wal.hpp"
#include "globj.hpp"
#include "extra.hpp"

#include <map>
#include <string>
#include <iostream>

class phonebook :public templet::globj {
public:
	phonebook(const char filename[]) :globj(filename) { init(); }
public:
	void put(const std::string& name, long phone) {
		update(_put, [&](std::ostream&out) {
			out << name << " " << phone;
			},
			[this](std::istream&in, std::ostream&) {
				std::string name; long phone; in >> name >> phone;
				book[name] = phone;
			});
	}
	std::map<std::string, long> book;
private:
	enum { _put };
	void on_init() override { put(std::string(), 0); }
};

int main()
{
    phonebook phbook("data.txt");
	
    templet::job job(3);
    
	job([&](unsigned pid) {

		if (pid == 1) {// user 1 'process'
			phbook.put(std::string("John"), 111111);
			phbook.put(std::string("Mary"), 333333);
		}
		else if (pid == 2) {// user 2 'process'
			phbook.put(std::string("John"), 222222);
			phbook.put(std::string("Bob"), 4444444);
		}

		if (pid == 0) {// in master 'process'
			templet::job::delay(1.0);
			phbook.update();
            std::cout << "Phone book:" << std::endl;
    		for (auto& it : phbook.book)
    			std::cout << it.first << "--" << it.second << std::endl;
		}
	});
    
	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
}
