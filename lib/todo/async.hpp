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
		async(wal&){}
        inline void task(
			std::function<void(std::ostream&)> action,
			std::function<void(std::istream&)> load
		){ async::task(false,action,load); }
		void task(bool local,
			std::function<void(std::ostream&)> action,
			std::function<void(std::istream&)> load
		){}
		void wait(){}
	};
}
