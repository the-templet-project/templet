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

	namespace meta {
		class state {
		public:
			void name(const char name[]) { _name = name; }
			void prefix(const char prefix[]) { _prefix = prefix; }
			
			enum action {_output,_update,_update_output,_save_update,_save_update_output};
			
			struct update {
				friend class state;
			private:
				update(const char name[], action act) :_name(name), _action(act) {}
			public:
				update& par(const char def[], const char use[], const char init[] = "") {
					_par_list.push_back(param(def, use, init));
					return *this;
				}
			private:
				struct param { 
					param(const char def[], const char use[], const char init[] = ""):_def(def),_use(use),_init(init){}
					std::string _def; std::string _use; std::string _init; 
				};
				std::list<param> _par_list;
				std::string _name; action _action;
			};
			
			update& def(const char name[], action act) { _updates.push_back(update(name,act)); return _updates.back(); }
		public:
			void print(std::ostream&out,bool markup=false){
				if (_prefix != "") out << _prefix << std::endl;
				out << "class " << _name << " : public templet::state {" << std::endl;
				out << "// TODO: " << _name <<"(... templet::write_ahead_log&l ...) : state(l) {...}" << std::endl;
				out << "// TODO: add methods and fields" << std::endl;
				out << "};"  << std::endl;
			}
		private:
			std::string _name;
			std::string _prefix;
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
