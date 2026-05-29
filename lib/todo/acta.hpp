/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include "wal.hpp"

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

    class acta {
	public:
		acta(wal&){}
		void run(){}
		inline void operator()() { acta::run(); }
		virtual void on_run() {}
	public:
		class actor;
		class message {
		public:
            message(){}
			message(actor&){}
			message(actor&, std::function<void()> action){}
			message(actor*){}
			message(actor*, std::function<void()> action){}
            void init(actor&){}
            void init(actor&, std::function<void()> action){}
            void init(actor*){}
            void init(actor*, std::function<void()> action){}
		public:
			bool operator()(){return false;};// is available now?
			void cli(actor*,std::function<void()> action){}
			void srv(actor*,std::function<void()> action){}
            void cli(actor&,std::function<void()> action){}
			void srv(actor&,std::function<void()> action){}
		};
	public:
		class actor {
		public:
            actor(){}
			actor(acta&,bool start=false){}
			actor(acta*,bool start = false){}
            void init(acta&,bool start = false){}
            void init(acta*,bool start = false){}
		public:
			void ask(message&){}
			void ask(message*){}
			void ret(message&){}
			void ret(message*){}
			void say(message&){}
			void say(message*){}
		public:
			void task(bool local,
				std::function<void(std::ostream&)> action,
				std::function<void(std::istream&)> load
			){}
			inline void task(
				std::function<void(std::ostream&)> action,
				std::function<void(std::istream&)> load
			) {
				actor::task(false,action,load);
			}
			void stop(){}
        protected:
            virtual void on_start(){}
		};
	};
}

