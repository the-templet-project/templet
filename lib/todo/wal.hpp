#pragma once

/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#include <functional>
#include <istream>
#include <ostream>
#include <mutex>
#include <cstdio>
#include <fstream>
#include <cassert>

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
			_log.push_back(std::pair<unsigned, std::string>(tag, blob));
			index = (unsigned)(_log.size() - 1);
		}
		bool read(unsigned index, unsigned& tag, std::string& blob) override {
			std::unique_lock<std::mutex> lock(_mut);
			if (index < _log.size()) { tag = _log[index].first; blob = _log[index].second; return true; }
			return false;
		}
		void print(std::ostream& out) {
			for (int i = 0; i < _log.size(); i++) {
				out << "index:" << i << " tag:" << _log[i].first << std::endl
					<< "entry:" << _log[i].second << std::endl;
			}
		}
	protected:
		std::vector<std::pair<unsigned, std::string>> _log;
		std::mutex _mut;
	};

    class config {
    public:
        config(const char[]){}
    };

    class intrapi{};
    
    class stub{
    friend class server;
    };

    class srvwal : public intrapi {
    public:
        srvwal(const config&){}
        void listen(){}
    };

    class proxy : public intrapi{
    friend class cliwal;
    };

    class cliwal : public wal /*, public objstore */{
    public:
        cliwal(srvwal&){}
        cliwal(const config&){}
    public:
        void write(unsigned& index, unsigned tag, const std::string& blob) override {
		}
		bool read(unsigned index, unsigned& tag, std::string& blob) override {
			return false;
		}
    };

}
