/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>
#include <mutex>
#include <cstdio>
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

	class globjwal :public wal {
	public:
		globjwal(const char filename[], bool lazy = true) :globjwal(std::string(filename), lazy) {}
		globjwal(const std::string& filename, bool lazy = true) :
			_current_index(0), _cashed_write(false), _filename(filename), _lazy(lazy) {
			_file = fopen(_filename.c_str(), "rb");
			if (!_file) {
				_file = fopen(_filename.c_str(), "ab");
				assert(_file && "filewal: cannot open log file");
				_initial_read = false;
			}
			else
				_initial_read = true;
		}
		~globjwal() { fclose(_file); }

		void write(unsigned& index, unsigned tag, const std::string& blob) override {
			assert(!_initial_read && "filewal: access pattern violated");

			size_t ret_code;
			unsigned ubuf[3];//index,tag,blob size

			ubuf[0] = _current_index; //index
			ubuf[1] = tag;   //tag
			ubuf[2] = blob.size(); //blob size

			ret_code = fwrite(ubuf, sizeof(ubuf), 1, _file);
			assert(ret_code == 1 && "filewal: write error");

			ret_code = fwrite(blob.c_str(), sizeof(char), ubuf[2], _file);
			assert(ret_code == ubuf[2] && "filewal: write error");

			if (!_lazy) fflush(_file);

			_cashed_tag = tag; _cashed_blob = blob; _cashed_write = true;
			index = _current_index; _current_index++;
		}
		bool read(unsigned index, unsigned& tag, std::string& blob) override {
			if (_initial_read) {
				assert(index == _current_index && "filewal: access pattern violated");

				size_t ret_code;
				unsigned ubuf[3];//index,tag,blob size

				ret_code = fread(ubuf, 1, sizeof(ubuf), _file);

				if (ret_code == 0 && feof(_file)) {
					fclose(_file);
					_file = fopen(_filename.c_str(), "ab");
					assert(_file && "filewal: cannot open log file");
					_initial_read = false;
					return false;
				}

				assert(ret_code == sizeof(ubuf) && "filewal: read error");
				assert(ubuf[0] == _current_index && "filewal: integrity is compromised");

				tag = ubuf[1];//tag
				blob.resize(ubuf[2]);//size

				ret_code = fread((void*)blob.c_str(), sizeof(char), ubuf[2], _file);//blob
				assert(ret_code == ubuf[2] && "filewal: read error");

				_current_index++;
				return true;
			}
			else {// !_initial_read (write)
				assert((index == _current_index || index == _current_index - 1)
					&& "filewal: access pattern violated");

				if (index == _current_index - 1 && _cashed_write) {
					tag = _cashed_tag; blob = _cashed_blob;
					return true;
				}
				return false;
			}
		}
	private:
		FILE*_file;
		std::string _filename;
		bool _initial_read;
		unsigned _current_index;
		bool _cashed_write;
		unsigned _cashed_tag;
		std::string _cashed_blob;
		bool _lazy;
	};

    class walbuf{};
    class walcfg{};

    class srvbuf: public walbuf{
    public:
        srvbuf(const walcfg&);
    };

    class stub{
    public:
        stub(const walcfg&,walbuf&);
    public:
        void run();
    };

    class srvwal{
    public:
        srvwal(const walcfg&cfg){
            _srvbuf = new srvbuf(cfg); _stub = new stub(cfg,*_srvbuf);
        }
        ~srvwal(){delete _srvbuf;delete _stub;}
    public:
        void run(){_stub->run();}
    private:
        stub* _stub;
        srvbuf* _srvbuf;
    };

    class proxy: public walbuf{
    public:
        proxy(const walcfg&);
    };

    class clibuf: public wal{
    public:
        clibuf(const walcfg&,walbuf&);
    public:
		void write(unsigned& index, unsigned tag, const std::string& blob) override {}
        bool read(unsigned index, unsigned& tag, std::string& blob) override { return false; }
    };

    class cliwal: public wal{
    public:
        cliwal(const walcfg&cfg){
            _proxy = new proxy(cfg); _clibuf = new clibuf(cfg,*_proxy);
        }
        ~cliwal(){delete _proxy;delete _clibuf;}
    public:
		void write(unsigned& index, unsigned tag, const std::string& blob) override {
            _clibuf->write(index,tag,blob);
        }
        bool read(unsigned index, unsigned& tag, std::string& blob) override { 
            return _clibuf->read(index,tag,blob); 
        }
    private:
        proxy* _proxy;
        clibuf* _clibuf;
    };
    
    class filewal: public wal{
    public:
        filewal(const walcfg&cfg){
            _srvbuf = new srvbuf(cfg); _clibuf = new clibuf(cfg,*_srvbuf);
        }
        ~filewal(){delete _srvbuf;delete _clibuf;}
    public:
		void write(unsigned& index, unsigned tag, const std::string& blob) override {
            _clibuf->write(index,tag,blob);
        }
        bool read(unsigned index, unsigned& tag, std::string& blob) override { 
            return _clibuf->read(index,tag,blob); 
        }
    private:
        srvbuf* _srvbuf;
        clibuf* _clibuf;
    };

}
