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
			std::ostringstream out;

			if (selected_tickets.find(user) != selected_tickets.end()) {
				int ticket_number = selected_tickets[user];
				out.clear();
				out << "User's" << user << " ticket number is " << ticket_number << "." << std::endl;
				write(out.str());
				return;
			}

			if (vacant_tickets.size() == 0) {
				write(std::string("No vacant tickets.\n"));
				return;
			}

			std::stringstream ios;
			outer(ios, [](std::ostream&out) {
				out << rand();
			});

			int random=0;
			ios >> random;

			int selected = random % vacant_tickets.size();
			auto it = vacant_tickets.begin();

			for (int i = 0; i != selected; i++, it++);
			int ticket_number = *it;

			vacant_tickets.erase(ticket_number);
			selected_tickets[user] = ticket_number;

			out.clear();
			out << "User's" << user << " ticket number is " << ticket_number << "." <<std::endl;
			write(out.str());
			return;
		}
		case PRINT_TICKETS:
		{
			write(std::string("List of selected tickets.\n"));
			for (auto& t : selected_tickets) {
				std::ostringstream out;
				out << "         Name:" << t.first << std::endl
					<< "Ticket number:" << t.second << std::endl << std::endl;
				write(out.str());
			}
			return;
		}
		}
	}
public:
	enum { GET_TICKET, PRINT_TICKETS };
	ticketchatbot(templet::write_ahead_log&l) :templet::chatbot(l) { srand((unsigned)time(NULL)); }
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

	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	std::ostringstream user; user << "user" << pid;
	ticketchatbot tbot(wal);
	tbot.run(user.str(), ticketchatbot::GET_TICKET);
	tbot.run(user.str(), ticketchatbot::PRINT_TICKETS);
	////////////////////////////////////////////////////
	}); for (auto& t : threads) t.join();
	std::cout << "Success!" << std::endl;
}
