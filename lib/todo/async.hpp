/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

	class async {
	public:
		async(wal&);
		void task(bool local,
			std::function<void(std::ostream&)> action,
			std::function<void(std::istream&)> load
		);
		void wait();
	};
}
