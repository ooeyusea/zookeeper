#ifndef __BUFFER_STEAM_H__
#define __BUFFER_STEAM_H__
#include "hnet.h"

namespace olib {
	template <int32_t size = 1024>
	class BufferFileStream {
	public:
		BufferFileStream(const std::string& path)
			: _pos(0)
			, _rdstate(std::ios_base::goodbit) {
			_fp = fopen(path.c_str(), "rb");
			if (!_fp)
				_rdstate |= std::ios_base::badbit;
		}

		~BufferFileStream() {
			if (_fp)
				fclose(_fp);
		}

		void read(char * buf, size_t len) {
			if (_size < _pos + len)
				read_from_stream();

			if (_size < _pos + len)
				_rdstate |= std::ios_base::badbit;
			else {
				memcpy(buf, _content + _pos, len);
				_pos += len;

				if (_pos == _size)
					_rdstate |= std::ios_base::eofbit;
			}
		}

		void read(std::string& buf, size_t len) {
			if (_size < _pos + len)
				read_from_stream();

			if (_size < _pos + len)
				_rdstate |= std::ios_base::badbit;
			else {
				buf.append(_content + _pos, len);
				_pos += len;

				if (_pos == _size)
					_rdstate |= std::ios_base::eofbit;
			}
		}

		void read(hyper_net::DeliverBuffer& buf, size_t len) {
			if (_size < _pos + len)
				read_from_stream();

			if (_size < _pos + len)
				_rdstate |= std::ios_base::badbit;
			else {
				buf.buff = _content + _pos;
				_pos += len;

				if (_pos == _size)
					_rdstate |= std::ios_base::eofbit;
			}
		}

		bool good() const { return _rdstate == 0; }
		bool fail() const { return (_rdstate & (std::ios_base::badbit | std::ios_base::failbit)) != 0; }
		bool bad() const { return (_rdstate & std::ios_base::badbit) != 0; }
		bool eof() const { return (_rdstate & std::ios_base::eofbit) != 0; }

	private:
		void read_from_stream() {
			if (_size > _pos) {
				::memmove(_content, _content + _pos, _size - _pos);
				_size -= _pos;
			}
			else
				_size = 0;
			_pos = 0;

			_size += fread(_content + _size, 1, size, _fp);
		}

	private:
		FILE * _fp = nullptr;

		char _content[size];
		size_t _pos = 0;
		size_t _size = 0;

		std::ios_base::iostate _rdstate;
	};
}

#endif //__BUFFER_STEAM_H__
