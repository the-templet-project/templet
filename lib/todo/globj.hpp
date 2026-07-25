#pragma once

/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#include "wal.hpp"

#include <functional>
#include <istream>
#include <ostream>
#include <sstream>
#include <map>
#include <mutex>

namespace templet {

    class globj;

	class globjwal :public wal {
    friend class globj;
    private:
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
        globjwal() :  _file(NULL){}
	   ~globjwal() { if(_file) fclose(_file); }
    private:
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

	class globj {
	public:
		globj(wal&w) :_wal(w), _wal_index(0), _is_init(false) {}
    public:
        globj(const std::string& filename, bool lazy = true) :
            _globjwal(filename,lazy), _wal(_globjwal), _wal_index(0), _is_init(false) {}
        globj(const char filename[], bool lazy = true) :
            _globjwal(filename,lazy), _wal(_globjwal), _wal_index(0), _is_init(false) {}
    public:
        void init() { _is_init = true; on_init(); _is_init = false; update(); }
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
		std::map<unsigned, std::function<void(std::istream&, std::ostream&)>> _updaters;
		std::mutex _mut;
    private:
        globjwal _globjwal;
	};

}
