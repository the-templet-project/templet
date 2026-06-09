#include "wal.hpp"
#include "globj.hpp"
#include "extra.hpp"

#include <map>
#include <string>
#include <iostream>

class phonebook :public templet::globj {
public:
	phonebook(templet::wal&l) :globj(l) { init(); }

	void put(const std::string& name, long phone) {
		update(_put, [&](std::ostream&out) {
			out << name << " " << phone;
			},
			[this](std::istream&in, std::ostream&) {
				std::string name; long phone; in >> name >> phone;
				book[name] = phone;
			});
	}
	bool get(const std::string& name, long& phone) {
		update();
		if (book.find(name) != book.end()) {
			phone = book[name];
			return true;
		}
		return false;
	}
	void print() {
		update();
		std::cout << "Phone book:" << std::endl;
		for (auto& it : book)
			std::cout << it.first << "--" << it.second << std::endl;
	}
private:
	std::map<std::string, long> book;
private:
	enum { _put };
	void on_init() override {
		put(std::string(), 0);
	}
};

int main()
{
	templet::memwal wal;
	templet::job job(3);

	job([&](unsigned pid) {

		phonebook book(wal);

		if (pid == 1) {// user 1 'process'
			book.put(std::string("John"), 111111);
			book.put(std::string("Mary"), 333333);
		}
		else if (pid == 2) {// user 2 'process'
			book.put(std::string("John"), 222222);
			book.put(std::string("Bob"), 4444444);
		}

		if (pid == 0) {// in master 'process'
			templet::job::delay(1.0);
			book.print();
		}
		});

	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
}
