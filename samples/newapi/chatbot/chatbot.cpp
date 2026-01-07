#include <iostream>

#include <syncmem.hpp>

int main()
{
	class ticketchatbot : public templet::chatbot {
		void on_run(unsigned topic, std::string&user) override {
			outer([]() {});
			putln(std::string("hello world!"));
			std::string str; getln(str);
		}
	public:
		ticketchatbot(templet::write_ahead_log&l) :templet::chatbot(l) {}
	};
	templet::write_ahead_log wal;
	ticketchatbot tbot(wal);
	std::string user("user");
	tbot.run(0,user);
}
