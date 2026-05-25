/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <string>

namespace templet {
	
	class job {
	public:
		job(unsigned size);
		void run(std::function<void(unsigned pid)> process);
		void delay(double seconds);
		double duration();
	};

	class terminal {
	public:
		enum role{client,server};
		terminal(role, const char inpipe[], const char outpipe[]);
		terminal(role, const std::string& inpipe, const std::string& outpipe);
	public:
		void run(unsigned topic);
		void run(std::function<void(unsigned topic)> action);
	public:
		void write(const std::string&);
		void readln(std::string&);
	};

}