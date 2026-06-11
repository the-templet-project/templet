/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

	class chat {
	public:
		chat(wal&);
		void run(const std::string& user);
		void operator()(const std::string& user) { run(user); }
		void update();
		virtual void on_run(const std::string& user) = 0;
		void say(std::function<void()>output_action);
		void ask(
			std::istream& input,
			std::function<void(std::ostream&)>input_action
		);
	};
}
