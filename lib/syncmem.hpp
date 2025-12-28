/*--------------------------------------------------------------------------*/
/*  Copyright 2025 Sergei Vostokin                                          */
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
		friend class shared_state;
	public:
		void write(unsigned& index, unsigned tag, const std::string& blob) {
			std::unique_lock<std::mutex> lock(mut);
			log.push_back(std::pair<unsigned, std::string>(tag, blob));
			index = log.size() - 1;
		}
		bool read(unsigned index, unsigned& tag, std::string& blob) {
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
		void update() {}
		void update(const unsigned id,
			std::function<void(void)>doupdate) {}
		void update(const unsigned id,
			std::function<void(std::ostream&)>saveparams,
			std::function<void(std::istream&)>doupdate) {}
	protected:
		virtual void register_updates() = 0;
	private:
		write_ahead_log& log;
	};

	class tasks {
	public:
		tasks(write_ahead_log&l) :log(l) {}
		void operator()(std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {}
		void operator()(bool condition,
			std::function<void(std::ostream&)>exec,
			std::function<void(std::istream&)>goon) {}
		void barrier() {}
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
