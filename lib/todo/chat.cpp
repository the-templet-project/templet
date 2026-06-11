#include "wal.hpp"
#include "chat.hpp"
#include "extra.hpp"

#include <random>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <iomanip>

const int NUM_STUDENTS = 150;
const int NUM_TICKETS = 100;

class ticketchat : public templet::chat {
	void on_run(const std::string&user) override {

		if (selected_tickets.find(user) != selected_tickets.end()) {
			int ticket_number = selected_tickets[user];
			say([&]() {
				std::cout << "User '" << user << "' ticket number is " << ticket_number << "." << std::endl;
				});
			return;
		}

		std::stringstream ios;
		ask(ios, [&](std::ostream&out) {
			out << _rand();
			});

		if (vacant_tickets.size() == 0) {
			say([&]() { std::cout << "No vacant tickets for user '" << user << "'." << std::endl; });
			return;
		}

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
	ticketchat(templet::wal&wal) :templet::chat(wal) {
		for (int i = 1; i <= NUM_TICKETS; i++) vacant_tickets.insert(i);
		std::random_device rd; _rand.seed(rd());
	}
public:
	std::set<int>         vacant_tickets;
	std::map<std::string, int> selected_tickets;
private:
	std::minstd_rand _rand;
};

int main()
{
	templet::memwal wal;
	templet::job job(NUM_STUDENTS);

	job([&](unsigned pid) {
		std::ostringstream user; user << "user" << std::setw(3) << std::setfill('0') << pid;
		ticketchat tbot(wal);
		tbot.run(user.str());
		});

	ticketchat tbot(wal);
	tbot.update();

	std::cout << std::endl << "List of selected tickets." << std::endl;
	for (auto& t : tbot.selected_tickets)
		std::cout << "name:'" << t.first << "' ticket number:" << t.second << std::endl;

	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
}
