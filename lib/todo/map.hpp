/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

	class map {
	public:
		map(wal&);
		void init(unsigned size);
		void operator()(
			std::function<void(unsigned size)> init,
			std::function<void(unsigned iter)> map,
			std::function<void(unsigned iter, std::ostream&, bool mapped)> save
			= [](unsigned, std::ostream&, bool) {},
			std::function<void(unsigned iter, std::istream&, bool mapped)> load
			= [](unsigned, std::istream&, bool) {}
		);
	};
}
