/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>
#include <mutex>
#include <cstdio>

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
		void print() {
			for (int i = 0; i < _log.size(); i++) {
				std::cout << "index:" << i << " tag:" << _log[i].first << std::endl
					<< "entry:" << _log[i].second << std::endl;
			}
		}
	protected:
		std::vector<std::pair<unsigned, std::string>> _log;
		std::mutex _mut;
	};

	class filewal :public wal {
	public:
		filewal(const char filename[], bool lazy = true) :filewal(std::string(filename), lazy) {}
		filewal(const std::string& filename, bool lazy = true) :
			_current_index(0), _cashed_index(UINT_MAX), _filename(filename), _lazy(lazy) {
			_file = fopen(_filename.c_str(), "rb");
			if (!_file) {
				_file = fopen(_filename.c_str(), "ab");
				assert(_file && "filewal: cannot open log file");
				_initial_read = false;
			}
			else
				_initial_read = true;
		}
		~filewal() { fclose(_file); }

		void write(unsigned& index, unsigned tag, const std::string& blob) override {
			assert(!_initial_read && "filewal: access pattern violated");
			assert(_current_index == index && "filewal: access pattern violated");

			size_t ret_code;
			unsigned ubuf[3];//index,tag,blob size

			ubuf[0] = index; //index
			ubuf[1] = tag;   //tag
			ubuf[2] = blob.size(); //blob size

			ret_code = fwrite(ubuf, sizeof(ubuf), 1, _file);
			assert(ret_code == 1 && "filewal: write error");

			ret_code = fwrite(blob.c_str(), sizeof(char), ubuf[2], _file);
			assert(ret_code == ubuf[2] && "filewal: write error");

			if (!_lazy) fflush(_file);

			_cashed_index = _current_index;
			_cashed_tag = tag;
			_cashed_blob = blob;
			_current_index++;
		}
		bool read(unsigned index, unsigned& tag, std::string& blob) override {
			if (_initial_read) {
				size_t ret_code;
				unsigned ubuf[3];//index,tag,blob size

				assert(index == _current_index && "filewal: access pattern violated");
				ret_code = fread(ubuf, 1, sizeof(ubuf), _file);

				if (ret_code == 0 && feof(_file)) {
					fclose(_file);
					_file = fopen(_filename.c_str(), "ab");
					assert(_file && "filewal: cannot open log file");
					_initial_read = false;
					return false;
				}

				if (ret_code != sizeof(ubuf) && feof(_file)) {
					fclose(_file);
					truncate_chunk(_filename, ret_code);
					_file = fopen(_filename.c_str(), "ab");
					assert(_file && "filewal: cannot open log file");
					_initial_read = false;
					return false;
				}

				assert(!ferror(_file) && "filewal: read error");
				assert(ubuf[0] == _current_index && "filewal: integrity is compromised");

				tag = ubuf[1];//tag
				blob.resize(ubuf[2]);//size
				ret_code = fread((void*)blob.c_str(), sizeof(char), ubuf[2], _file);//blob

				if (ret_code != ubuf[2] && feof(_file)) {
					fclose(_file);
					truncate_chunk(_filename, 3 * (sizeof ubuf[0]) + ret_code * sizeof(char));
					_file = fopen(_filename.c_str(), "ab");
					assert(_file && "filewal: cannot open log file");
					_initial_read = false;
					return false;
				}

				assert(!ferror(_file) && "filewal: read error");
				_current_index++;
				return true;
			}
			else {
				assert(_cashed_index != UINT_MAX && "filewal: access pattern violated");
				assert((index == _cashed_index || index == _cashed_index + 1) && "filewal: access pattern violated");

				if (index == _cashed_index) {
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
		unsigned _cashed_index;
		unsigned _cashed_tag;
		std::string _cashed_blob;
		bool _lazy;
	private:
		void truncate_chunk(const std::string& filename, unsigned n) {
#if (__cplusplus>=201703L)
			std::filesystem::path p = filename;
			std::filesystem::resize_file(p, std::filesystem::file_size(p) - n);
#else
			assert(!"filewal: the last entry is corrupted");
#endif
		}
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
