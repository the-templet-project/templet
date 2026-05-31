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
		virtual void write(unsigned& index, unsigned tag, const std::string& blob) = 0;
		virtual bool read(unsigned index, unsigned& tag, std::string& blob) = 0;
	};

	class memwal :public wal {
	public:
		void write(unsigned& index, unsigned tag, const std::string& blob) override {
			std::unique_lock<std::mutex> lock(_mut);
			log.push_back(std::pair<unsigned, std::string>(tag, blob));
			index = (unsigned)(_log.size() - 1);
		}
		virtual bool read(unsigned index, unsigned& tag, std::string& blob) override {
			std::unique_lock<std::mutex> lock(_mut);
			if (index < _log.size()) { tag = _log[index].first; blob = _log[index].second; return true; }
			return false;
		}
	protected:
		std::vector<std::pair<unsigned, std::string>> _log;
		std::mutex _mut;
	};

	class cliwal :public wal {
	private:
		class proxy :public wal {};
	};

	class stub {
	private:
		class srvwal :public wal {};
	};
}
