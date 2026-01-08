#include <iostream>
#include <thread>
#include <atomic>

#define  TEMPLET_CHATBOT_TEST_IMPL
#include <syncmem.hpp>

class ticketchatbot : public templet::chatbot {
	void on_run(const std::string&user, unsigned topic) override {
		switch (topic) {
		case GET_TICKET:
		{
			if (selected_tickets.find(user) != selected_tickets.end()) {
				int ticket_number = selected_tickets[user];
				outer([&]() {
					std::cout << "User's" << user << " ticket number is " << ticket_number << "." << std::endl;
				});
				return;
			}

			if (vacant_tickets.size() == 0) {
				outer([&]() { std::cout << "No vacant tickets." << std::endl; });
				return;
			}

			std::stringstream ios;
			outer(ios, [](std::ostream&out) {	
				srand((unsigned)time(NULL));
				out << rand();
			});

			int random; ios >> random;

			int selected = random % vacant_tickets.size();
			auto it = vacant_tickets.begin();

			for (int i = 0; i != selected; i++, it++);
			int ticket_number = *it;

			vacant_tickets.erase(ticket_number);
			selected_tickets[user] = ticket_number;

			outer([&]() {
				std::cout << "User's " << user << " ticket number is " << ticket_number << "." << std::endl;
			});
			return;
		}
		case PRINT_TICKETS:
		{
			outer([]() {
				std::cout << std::endl << "List of selected tickets." << std::endl;
			});
			for (auto& t : selected_tickets) {
				outer([&]() {	
				std::cout << "         Name:" << t.first << std::endl
						  << "Ticket number:" << t.second << std::endl << std::endl;
				});
			}
			return;
		}
		}
	}
public:
	enum { GET_TICKET, PRINT_TICKETS };
	ticketchatbot(templet::write_ahead_log&l) :templet::chatbot(l) { 
		for (int i = 1; i <= 5; i++) vacant_tickets.insert(i);
	}
private:
	std::set<int>         vacant_tickets;
	std::map<std::string, int> selected_tickets;
};

const int NUM_THREADS = 10;
const int ARRAY_SIZE = 10;

int main()
{
	std::atomic_int PID = 0;

	templet::write_ahead_log wal;
	std::vector<std::thread> threads(NUM_THREADS);

	ticketchatbot tbot(wal);

	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	std::ostringstream user; user << "user" << pid;
	//ticketchatbot tbot(wal);
	tbot.run(user.str(), ticketchatbot::GET_TICKET);
	////////////////////////////////////////////////////
	}); for (auto& t : threads) t.join();
	
	tbot.run(std::string("user"), ticketchatbot::PRINT_TICKETS);
	
	std::cout << "Success!" << std::endl;
}
