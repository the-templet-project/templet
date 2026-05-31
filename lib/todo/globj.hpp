/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include "wal.hpp"

#include <functional>
#include <istream>
#include <ostream>
#include <sstream>
#include <map>
#include <mutex>

namespace templet {

	class globj {
	public:
		globj(wal&w):_wal(w), _wal_index(0), _is_init(false) {}
		void init() { _is_init = true; on_init(); _is_init = false; }
	protected:
		virtual void on_init() = 0;
	public:
		void update(
			unsigned id,
			std::function<void(std::ostream&)> save,
			std::function<void(std::istream&, std::ostream&)> update,
			std::function<void(std::istream&)> load = [](std::istream&) {}
		) {
			if (_is_init)
				_updaters[id] = update;
			else {
				std::unique_lock<std::mutex> lock(_mut);

				std::ostringstream out; unsigned index;
				save(out); _wal.write(index, id, out.str()); out.clear();

				unsigned tag; std::string blob;
				for (; _wal_index < index && _wal.read(_wal_index, tag, blob); _wal_index++) {
					auto& updater = _updaters[tag];
					{
						std::istringstream in(blob);
						updater(in, out); out.clear();
					}
				}
				_wal.read(_wal_index, tag, blob); _wal_index++;
				{
					auto& updater = _updaters[tag];
					std::istringstream in(blob); std::stringstream out;
					updater(in, out); load(out);
				}
			}
		}
		inline void update(
			unsigned id,
			std::function<void(std::istream&, std::ostream&)> update,
			std::function<void(std::istream&)> load = [](std::istream&) {}
		) {
			globj::update(id, [](std::ostream&) {}, update, load);
		}
		void update() {
			std::unique_lock<std::mutex> lock(_mut);

			unsigned tag; std::string blob;
			std::ostringstream out;

			for (; _wal.read(_wal_index, tag, blob); _wal_index++) {
				auto& updater = _updaters[tag];
				{ 
					std::istringstream in(blob); 
					updater(in, out); out.clear();
				}
			}
		}
	private:
		wal& _wal;
		unsigned _wal_index;
		bool _is_init;
		std::map<unsigned,std::function<void(std::istream&, std::ostream&)>> _updaters;
		std::mutex _mut;
	};
}
