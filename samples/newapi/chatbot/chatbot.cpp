#include <iostream>
#include <thread>
#include <atomic>

//#define  TEMPLET_CHATBOT_TEST_IMPL
#include <syncmem.hpp>

const int NUM_STUDENTS = 5;
const int NUM_TICKETS = 5;

class ticketchatbot : public templet::chatbot {
	void on_chat(const std::string&user) override {
		if (selected_tickets.find(user) != selected_tickets.end()) {
			int ticket_number = selected_tickets[user];
			say([&]() {
				std::cout << "User '" << user << "' ticket number is " << ticket_number << "." << std::endl;
			});
			return;
		}

		if (vacant_tickets.size() == 0) {
			say([&]() { std::cout << "No vacant tickets for user '"<< user << "'." << std::endl; });
			return;
		}

		std::stringstream ios;
		ask(ios, [&](std::ostream&out) {	
			srand((unsigned)time(NULL));
			out << rand()<< " " <<user;
		});

		int random; ios >> random;

		int selected = random % vacant_tickets.size();
		auto it = vacant_tickets.begin();

		for (int i = 0; i != selected; i++, it++);
		int ticket_number = *it;

		vacant_tickets.erase(ticket_number);
		selected_tickets[user] = ticket_number;

		say([&]() {
			std::cout << "User '" << user << "' ticket number is " << ticket_number << "." << std::endl;
		});
	}
public:
	ticketchatbot(templet::write_ahead_log&l) :templet::chatbot(l) { 
		for (int i = 1; i <= NUM_TICKETS; i++) vacant_tickets.insert(i);
	}
public:
	std::set<int>         vacant_tickets;
	std::map<std::string, int> selected_tickets;
};

int main()
{
	templet::scheduler s;
	templet::scheduler::worker* w1;
	templet::scheduler::worker* w2;

	for(int i=0; i<100; i++)
	{
		{
			std::cout << "#" << i + 1 << std::endl;
			w1 = s.new_worker();
			std::thread t1([&]() {
				std::cout << "--1--   0" << std::endl;
				s.worker_enter();
				std::cout << "--1--   1" << std::endl;
				s.worker_next();
				std::cout << "--1--   2" << std::endl;
				s.worker_leave();
				std::cout << "--1--   3" << std::endl;
			});
			s.master_enter(w1);

			w2 = s.new_worker();
			std::thread t2([&]() {
				std::cout << "--2--   0" << std::endl;
				s.worker_enter();
				std::cout << "--2--   1" << std::endl;
				s.worker_next();
				std::cout << "--2--   2" << std::endl;
				s.worker_leave();
				std::cout << "--2--   3" << std::endl;
			});
			s.master_enter(w2);

			s.master_next(w2);
			s.master_next(w1);
			s.master_next(w1);
			s.master_next(w2);

			s.master_leave(w2);
			t2.join();
			s.del_worker(w2);

			s.master_leave(w1);
			t1.join();
			s.del_worker(w1);
		}
		std::cout << std::endl;
	}
}

/*
int main()
{
	std::atomic_int PID = 0;

	templet::write_ahead_log wal;

	std::vector<std::thread> threads(NUM_STUDENTS);
	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	std::ostringstream user; user << "user" << pid;
	ticketchatbot tbot(wal);
	tbot.chat(user.str());
	////////////////////////////////////////////////////
	}); for (auto& t : threads) t.join();

	ticketchatbot tbot(wal);
	tbot.update();

	std::cout << std::endl << "List of selected tickets." << std::endl;
	for (auto& t : tbot.selected_tickets)
		std::cout << "name:'" << t.first << "' ticket number:" << t.second << std::endl;
}
*/