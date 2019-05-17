#ifndef __LOCAL_FILE_H__
#define __LOCAL_FILE_H__
#include "hnet.h"
#include "RefObject.h"

namespace ofs {
	class LocalFile : public RefObject {
	public:
		LocalFile(const std::string& path) : _path(path), _mutex(true) {}
		LocalFile(std::string&& path) : _path(path), _mutex(true) {}
		~LocalFile() {}

		inline static LocalFile * Create(const std::string& path) {
			return new LocalFile(path);
		}
		inline void Release() { delete this; }

		inline bool Active() {
			Acquire();
			_tick = GetTickCount();
			++_usedCount;
		}

		inline bool Less(LocalFile * other) {
			return _tick < other->_tick || (_tick == other->_tick && _usedCount < other->_usedCount);
		}

		bool Read(int32_t offset, int32_t size, std::string& data);
		bool Write(int32_t offset, const char * data, int32_t size);

	private:
		std::string _path;
		int64_t _tick = 0;
		int32_t _usedCount = 0;

		hn_shared_mutex _mutex;
	};
}

#endif //__LOCAL_FILE_H__
