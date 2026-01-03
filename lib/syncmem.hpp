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
		state(write_ahead_log&l) :log(l) {}
		void init() {}
		void update() {}
		void update(const unsigned id,
			std::function<void(void)>update) {}
		void update(const unsigned id,
			std::function<void(std::ostream&)>save,
			std::function<void(std::istream&)>update) {}
	protected:
		virtual void on_init() = 0;
	private:
		write_ahead_log& log;
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

			void skel(const char skel_file[]) { _skel_file = skel_file; }

			void file(const char module_file[]) {
				std::string command;

				if (system(NULL) == 0) {
					std::cout << "ERROR: command processor is not detected";
					exit(EXIT_FAILURE);
				};

				command = std::string(_skel_file)+" -i " + module_file;

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

				command = _skel_file + " -i " + module_file + " -s " + module_file + ".cgn";

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

				out << "struct meta_" << _name << " : public templet::meta::state {" << std::endl;
				out << "\tmeta_" << _name << "() {" << std::endl;

				out << "/*$TET$$meta*/" << std::endl;
				out << "// TODO: put state class defs here" << std::endl;
				out << "/*$TET$*/" << std::endl;

				out << "\tfile(__FILE__);" << std::endl;
				out << "}};" << std::endl;
				out << std::endl;

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
			std::string _skel_file;
			std::list<update> _updates;
		};
	}

	class task_engine {
	public:
		task_engine(write_ahead_log&l) :log(l) {}
		void async(std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {}
		void async(bool condition,
			std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {}
		void await() {}
	private:
		write_ahead_log& log;
	};

	class syncmem_engine {
	};

	class syncmem_task : task {
	public:
		syncmem_task(actor*a, task_adaptor ta) {}
	};

}
