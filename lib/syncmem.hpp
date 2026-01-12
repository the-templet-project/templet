/*--------------------------------------------------------------------------*/
/*  Copyright 2025-2026 Sergei Vostokin                                     */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/
#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <map>
#include <set>
#include <cassert>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#include "templet.hpp"

namespace templet {

	class write_ahead_log {
	public:
		virtual void write(unsigned& index, unsigned tag, const std::string& blob) {
			std::unique_lock<std::mutex> lock(mut);
			log.push_back(std::pair<unsigned, std::string>(tag, blob));
			index = log.size() - 1;
		}
		virtual bool read(unsigned index, unsigned& tag, std::string& blob) {
			std::unique_lock<std::mutex> lock(mut);
			if (index < log.size()) { tag = log[index].first; blob = log[index].second; return true; }
			return false;
		}
	private:
		std::vector<std::pair<unsigned, std::string>> log;
		std::mutex mut;
	};

#ifdef  TEMPLET_STATE_TEST_IMPL
	class state {
	public:
		state(write_ahead_log&) {}
		void init() {}
		void update() {}
		void update(const unsigned id,
			std::function<void(void)>_update) {
			_update();
		}
		void update(const unsigned id,
			std::function<void(std::ostream&)>_save,
			std::function<void(std::istream&)>_update) {
			std::stringstream ios;
			_save(ios); _update(ios);
		}
	protected:
		virtual void on_init() = 0;
	};
#else //TEMPLET_STATE_WORK_IMPL
	class state {
	public:
		state(write_ahead_log&l) :wal(l), wal_index(0), is_init(false) {}
		void init() { is_init = true; on_init(); is_init = false; }

		void update() {
			unsigned tag; std::string blob;
			for (; wal.read(wal_index, tag, blob); wal_index++) {
				changer& changer = changers[tag];
				if (changer.use_input) { std::istringstream in(blob); changer.on_update_input(in); }
				else changer.on_update();
			}
		}
		void update(const unsigned id,
			std::function<void(void)>update) {
			if (is_init) 
				changers[id] = changer(update);
			else {
				std::string empty; unsigned index;
				wal.write(index, id, empty);
				state::update(index);
			}
		}
		void update(const unsigned id,
			std::function<void(std::ostream&)>save,
			std::function<void(std::istream&)>update) {
			if (is_init)
				changers[id] = changer(update);
			else {
				std::ostringstream out; unsigned index;
				save(out); wal.write(index, id, out.str());
				state::update(index);
			}
		}
	protected:
		virtual void on_init() = 0;
	private:
		void update(unsigned index) {
			unsigned tag; std::string blob;
			for (; wal_index <= index && wal.read(wal_index, tag, blob); wal_index++) {
				changer& changer = changers[tag];
				if (changer.use_input) { std::istringstream in(blob); changer.on_update_input(in); }
				else changer.on_update();
			}
		}
		struct changer {
			changer(std::function<void(std::istream&)>update) :on_update_input(update), use_input(true){}
			changer(std::function<void(void)>update) :on_update(update), use_input(false) {}
			changer() : use_input(false), on_update_input([](std::istream&){}), on_update([](){}){}
			bool use_input;
			std::function<void(std::istream&)>on_update_input;
			std::function<void(void)>on_update;
		};
		write_ahead_log& wal;
		unsigned wal_index;
		std::map<unsigned,changer> changers;
		bool is_init;
	};
#endif

	namespace meta {
		class state {
		protected:
			void name(const char name[]) { _name = name; }
			void prefix(const char prefix[]) { _prefix = prefix; }
			
			enum action {_output,_update,_update_output,_save_update,_save_update_output};
			
			struct update {
				friend class state;
			private:
				update(const char name[], action act) :_name(name), _action(act), _return("void") {}
				update(const char ret[], const char name[], action act) :_return(ret), _name(name), _action(act) {}
			public:
				update& par(const char def[], const char use[], const char init[] = "") {
					_par_list.push_back(param(def, use, init));
					return *this;
				}
			private:
				void print_init(std::ostream& out, std::string& class_name) {
					for (param& p : _par_list) {
						if(p._init != "") out << p._init << "; ";
					}

					out << class_name << "::" << _name << "(";
					bool first = true;
					for (param& p:_par_list) {
						if (first)first = false; else out << ", ";
						out << p._use;
					}
					out << ");";
				}
				void print_param(std::ostream& out) {
					bool first = true;
					for (param& p : _par_list) {
						if (first)first = false; else out << ", ";
						out << p._def;
					}
				}
				struct param { 
					param(const char def[], const char use[], const char init[] = ""):_def(def),_use(use),_init(init){}
					std::string _def; std::string _use; std::string _init; 
				};
				std::list<param> _par_list;
				std::string _name; std::string _return; action _action;
			};
			
			update& def(const char name[], action act) 
				{ _updates.push_back(update(name,act)); return _updates.back(); }
			update& def(const char ret[], const char name[], action act) 
				{ _updates.push_back(update(ret, name, act)); return _updates.back(); }

			void file(const char module_file[]) {
				if (system(NULL) == 0) {
					std::cout << "ERROR: command processor is not detected";
					exit(EXIT_FAILURE);
				};

				std::string skel_file(__FILE__);
				char delim = skel_file.at(skel_file.size() - (strlen("syncmem.hpp")+1));
				auto pos = skel_file.find(std::string("lib") + delim + "syncmem.hpp");
				skel_file.replace(pos, skel_file.size(),std::string("bin")+delim+"skel");

#if defined(_WIN32) || defined(_WIN64) 
				skel_file.append(".exe");
#endif

				std::string command;
				command = std::string(skel_file)+" -i " + module_file;

				if (system(command.c_str()) == EXIT_FAILURE) {
					std::cout << "ERROR: skel processor or input file is not detected";
					exit(EXIT_FAILURE);
				};

				{
					std::ofstream gen_file(std::string(module_file) + ".cgn");
					if (!gen_file) {
						std::cout << "ERROR: open '"<< module_file << ".cgn" <<"' file for writing";
						exit(EXIT_FAILURE);
					}
					state::print(gen_file);
				}

#if defined(_WIN32) || defined(_WIN64) 
				command = std::string("copy ") + module_file + " " + module_file + ".bak";
#else
				command = std::string("cp ") + module_file + " " + module_file + ".bak";
#endif

				if (system(command.c_str()) == EXIT_FAILURE) {
					std::cout << "ERROR: failed to backup " << module_file << " to '" << module_file << ".bak'";
					exit(EXIT_FAILURE);
				}

				command = skel_file + " -i " + module_file + " -s " + module_file + ".cgn";

				int ret;
				if (ret = system(command.c_str()) == EXIT_FAILURE) {
#if defined(_WIN32) || defined(_WIN64)
					command = std::string("copy ") + module_file + ".bak" + " " + module_file;
#else	
					command = std::string("cp ") + module_file + ".bak" + " " + module_file;
#endif
					system(command.c_str());
				}

#if defined(_WIN32) || defined(_WIN64)
				command = std::string("del ") + module_file + ".cgn";
#else
				command = std::string("rm ") + module_file + ".cgn";
#endif
				system(command.c_str());

				if (ret == EXIT_SUCCESS) std::cout << "Ok";

				exit(ret);
			}

		private:
			void print(std::ostream&out){
				out << "/*$TET$$header*/" << std::endl;
				out << "#include <syncmem.hpp>" << std::endl;
				out << "/*$TET$*/" << std::endl << std::endl;

				if (_prefix != "") out << _prefix << std::endl;
				out << "class " << _name << " : public templet::state {" << std::endl;
				print_todo("header$", out, std::string(_name)+"(...templet::write_ahead_log&l...) : state(l) {...init();...}");

				for (update& upd : _updates) {
					out << "\t"<< upd._return << " " << upd._name << "(";
					upd.print_param(out);
					out << ") {" << std::endl;

					if (upd._action == _save_update || upd._action == _save_update_output) {
						out << "\t\tupdate(_" << upd._name << ", [&](std::ostream&out) {" << std::endl;
						print_todo(std::string(upd._name) + "$save", out, "put variable saving code here");
						out << "\t\t}," << std::endl;
						out << "\t\t[this](std::istream&in) {" << std::endl;
						print_todo(std::string(upd._name) + "$update", out, "put update code here");
						out << "\t\t});" << std::endl;
					}
					else if (upd._action == _update || upd._action == _update_output) {
						out << "\t\tupdate(_" << upd._name << ", [this]() {" << std::endl;
						print_todo(std::string(upd._name) + "$update", out, "put update code here");
						out << "\t\t});" << std::endl;
					}
					else if (upd._action == _output) {
						out << "\t\tupdate();" << std::endl;
					}

					if (upd._action == _output || upd._action == _update_output || upd._action == _save_update_output) {
						print_todo(std::string(upd._name) + "$output", out, "put output code here");
					}

					out << "\t}" << std::endl;
				}

				out << "private:" << std::endl;
				out << "\tenum {" << std::endl;

				bool first = true;
				for (update& upd:_updates) {
					if (upd._action == _update || 
						upd._action == _update_output ||
						upd._action == _save_update ||
						upd._action == _save_update_output) {
						if (first) first = false; else out << "," << std::endl;
						out << "\t\t_" << upd._name; 
					}
				}
				out << std::endl << "\t};" << std::endl;
				
				out << "\tvoid on_init() override {" << std::endl;
				for (update& upd : _updates) {
					if (upd._action == _update ||
						upd._action == _update_output ||
						upd._action == _save_update ||
						upd._action == _save_update_output) {
						out << "\t\t{ "; upd.print_init(out, _name); out << " }" << std::endl;
					}
				}
				out << "\t}" << std::endl;

				print_todo("footer$", out, "add methods and fields here");

				out << "};"  << std::endl;

				out << std::endl;
				out << "/*$TET$$footer*/" << std::endl;
				out << "/*$TET$*/" << std::endl << std::endl;
			}
		private:
			void print_todo(const std::string&id, std::ostream&out, const std::string&todo) {
				out << "/*$TET$"<< _name << "$" << id <<  "*/" << std::endl;
				out << "// TODO: " << todo << std::endl;
				out << "/*$TET$*/" << std::endl;
			}
		private:
			std::string _name;
			std::string _prefix;
			std::list<update> _updates;
		};
	}

#ifdef  TEMPLET_TASK_TEST_IMPL
	class task_engine {
	public:
		task_engine(write_ahead_log&) : in_exec(0), in_goon(0), in_await(0) {}
		void async(std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {
			assert(!in_await || (in_goon && !in_exec));
			task_pool.push_back(task(exec, goon));
		}
		void async(bool condition,
			std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {
			assert(!in_await || (in_goon && !in_exec));
			task_pool.push_back(task(exec, goon));
		}
		void await() {
			assert(!in_await);
			in_await++;
			for (task& t : task_pool) {
				std::stringstream sstr;
				in_exec++; t.exec(sstr); in_exec--;
				in_goon++; t.goon(sstr); in_goon--;
			}
			task_pool.clear();
			in_await--;
		}
	private:
		struct task {
			task(std::function<void(std::ostream&)>e, 
				std::function<void(std::istream&)>g):exec(e), goon(g){}
			std::function<void(std::ostream&)> exec;
			std::function<void(std::istream&)> goon;
		};
		std::list<task> task_pool;
	private:
		int in_exec;
		int in_goon;
		int in_await;
	};
#else
	class task_engine {
	public:
		task_engine(write_ahead_log&l): 
			wal(l), wal_index(0), task_index(0), in_exec(0), in_goon(0), in_await(0) {
			srand((unsigned)time(NULL));
		}
		void async(std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {
			assert(!in_await || (in_goon && !in_exec));
			task_pool[task_index] = task(false,false,exec,goon);
			ready_tasks.insert(task_index);
			task_index++;
		}
		void async(bool condition,
			std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {
			assert(!in_await || (in_goon && !in_exec));
			task_pool[task_index] = task(true, condition, exec, goon);
			if(condition)ready_tasks.insert(task_index);
			task_index++;
		}
		void await() {
			assert(!in_await);
			in_await++;

			bool timeout_needed = false;
			for (;;) {
				unsigned tag; std::string blob;

				if (timeout_needed) {
					std::this_thread::sleep_for(100ms);
					timeout_needed = false;
				}

				for (; wal.read(wal_index, tag, blob); wal_index++) {
					if (task_pool.find(tag) != task_pool.end()) {
						task& t = task_pool[tag];
						std::istringstream in(blob);
						t.goon(in);
						ready_tasks.erase(tag);
						task_pool.erase(tag);
					}
				}

				if (task_pool.size() == 0) break;

				if (ready_tasks.size() != 0) {
					int selected = rand() % ready_tasks.size();
					auto it = ready_tasks.begin(); for (int i = 0; i != selected; i++, it++) {} tag = *it;

					std::ostringstream out;
					task_pool[tag].exec(out);
					unsigned indx; wal.write(indx, tag, out.str());
					ready_tasks.erase(tag);
				}
				else timeout_needed = true;
			}
			in_await--;
		}
	private:
		struct task {
			task(bool localization, bool local, 
				std::function<void(std::ostream&)>ex,
				std::function<void(std::istream&)>go): 
				is_localization(localization), is_local(local), exec(ex), goon(go){}
			task() : is_localization(false), is_local(false), 
				exec([](std::ostream&) {}), goon([](std::istream&) {}) {}
			bool is_local;
			bool is_localization;
			std::function<void(std::ostream&)> exec;
			std::function<void(std::istream&)> goon;
		};
		std::map<unsigned, task> task_pool;
		std::set<unsigned> ready_tasks;
		unsigned task_index;
	private:
		int in_exec;
		int in_goon;
		int in_await;
	private:
		write_ahead_log& wal;
		unsigned wal_index;
	};
#endif

	class syncmem_task;

	class syncmem_engine {
		friend class syncmem_task;
	public:
		syncmem_engine(write_ahead_log&l):tsk_eng(l){}
		void run(){ tsk_eng.await(); }
	private:
		void submit(syncmem_task&t);
	private:
		task_engine tsk_eng;
	};

	class syncmem_task : task {
		friend	class syncmem_engine;
	public:
		syncmem_task(actor*a, task_adaptor ta) : _actor(a), _tsk_adaptor(ta), _eng(0), _is_idle(true) {}
		void engine(syncmem_engine&e) { assert(_eng == 0); _eng = &e; }
		void submit(std::function<void(std::ostream&)>exec) 
		{
			_exec = exec; _localized = false; _local = false;
			_actor->suspend(); _eng->submit(*this);
		}
		void submit(bool cond,std::function<void(std::ostream&)>exec) 
		{
			_exec = exec; _localized = true; _local = cond;
			_actor->suspend(); _eng->submit(*this);
		}
		std::istream& operator()() { return (*_result); }

	private:
		actor*       _actor;
		task_adaptor _tsk_adaptor;

		syncmem_engine* _eng;
		bool _is_idle;

		bool _localized;
		bool _local;
		std::function<void(std::ostream&)> _exec;
		std::istream* _result;
	};

	void syncmem_engine::submit(syncmem_task&t) {
		assert(t._is_idle);
		if (t._localized) {
			tsk_eng.async(t._local,t._exec,
				[&t](std::istream&in) {
				t._result = &in;
				(*t._tsk_adaptor)(t._actor, &t);
				t._is_idle = true;
				t._actor->resume();
			});
		}
		else {
			tsk_eng.async(t._exec,
				[&t](std::istream&in) {
				t._result = &in;
				(*t._tsk_adaptor)(t._actor, &t);
				t._is_idle = true;
				t._actor->resume();
			});
		}
	}

#ifdef  TEMPLET_CHATBOT_TEST_IMPL
	class chatbot {
	protected:
		chatbot(write_ahead_log&l): _run(false),_outer(false) {}
	public:
		bool chat(const std::string&with_user, unsigned on_topic) {
			std::unique_lock<std::mutex> lock(mut);
			_run = true; on_chat(with_user, on_topic); _run = false;
			return true;
		}//chat on the topic if chat has not already started
		bool chat(const std::string&with_user) {
			return false;
		}//continue the chat on the previous topic if it was started
		void update() { assert(!_outer); }
	protected:
		virtual void on_chat(const std::string&with_user, unsigned on_topic) = 0;
	protected:
		void say(std::function<void()>f) { 
			assert(_run && !_outer); _outer = true; f(); _outer = false; 
		}
		void ask(std::ostream&out,std::function<void(std::ostream&)>f) {
			assert(_run && !_outer); _outer = true; f(out); _outer = false;
		}
	private:
		bool _run;
		bool _outer;
		std::mutex mut;
	};
#else
	class chatbot {
	protected:
		chatbot(write_ahead_log&l)  {}
	public:
		bool chat(const std::string&with_user, unsigned on_topic) {
			return false;
		}//chat on the topic if chat has not already started
		bool chat(const std::string&with_user) {
			return false;
		}//continue the chat on the previous topic if it was started
		void update() {  
		}
	protected:
		virtual void on_chat(const std::string&with_user, unsigned on_topic) = 0;
	protected:
		void say(std::function<void()>f) {
			f();
		}
		void ask(std::ostream&out, std::function<void(std::ostream&)>f) {
			f(out);
		}
	private:
		struct session{
			session(chatbot&cb) : cbot(cb) {}
			void open(unsigned action_id) {}
			void goon(unsigned action_id) {}
			void close() {}
			chatbot& cbot;
		};
		std::map<std::string,session> sessions;
		std::map<unsigned,session*> actions;
	};
#endif
}
