/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

	class acta {
	public:
		acta(wal&);
		void run();
		virtual void on_run() {}
	public:
		class actor;
		class message {
		public:
			message(actor&);
			message(actor*);
		public:
			bool operator()(actor*);// is available now?
			bool operator()(actor&);
		};
	public:
		class actor {
		public:
			actor(acta&);
			actor(acta*);
		public:
			void ask(message&);
			void ask(message*);
			void ret(message&);
			void ret(message*);
			void say(message&);
			void say(message*);
		public:
			void task(bool local,
				std::function<void(std::ostream&)> action,
				std::function<void(std::istream&)> load
			);
		};
	};
}
