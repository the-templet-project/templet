/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>
#include <mutex>

namespace templet {

	class wal {
	public:
		virtual void write(unsigned& index, unsigned tag, const std::string& blob){
            std::unique_lock<std::mutex> lock(mut);
			log.push_back(std::pair<unsigned, std::string>(tag, blob));
			index = (unsigned)(log.size() - 1);
        }
		virtual bool read(unsigned index, unsigned& tag, std::string& blob){
            std::unique_lock<std::mutex> lock(mut);
			if (index < log.size()) { tag = log[index].first; blob = log[index].second; return true; }
			return false;
        }
	protected:
		std::vector<std::pair<unsigned, std::string>> log;
		std::mutex mut;
	};

	class memwal :public wal {};

	class cliwal :public wal {
	private:
		class proxy :public wal {};
	};

	class stub {
	private:
		class srvwal :public wal {};
	};
}
