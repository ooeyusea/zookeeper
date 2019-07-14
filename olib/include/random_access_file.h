#ifndef __RANDOM_ACCESS_FILE_H__
#define __RANDOM_ACCESS_FILE_H__
#include "hnet.h"
#include "time_helper.h"

namespace olib {
	enum class RandomAccessFileResult : int8_t {
		SUCCESS,
		FILE_OPEN_FAILED,
		OP_FAILED,
		OP_INCOMPELETE,
		OP_OUT_OF_RANGE,
	};

	class RandomAccessFile {
	public:
		RandomAccessFile(const char* path, const char* mode = "rw");
		~RandomAccessFile();

		RandomAccessFileResult Read(int64_t offset, char * buf, int64_t size);
		RandomAccessFileResult Write(int64_t offset, const char* buf, int64_t size);
		RandomAccessFileResult Append(const char* buf, int64_t size);

	private:
		inline bool IsOpened() {
#ifdef WIN32
			return _fd != INVALID_HANDLE_VALUE;
#else
			return _fd > 0;
#endif
		}

		bool Open(int32_t * fileSize);

	private:
#ifdef WIN32
		HANDLE _fd = INVALID_HANDLE_VALUE;
#else
		int32_t _fd = -1;
#endif
		std::string _path;
		std::string _mode;
	};
}

#endif //__RANDOM_ACCESS_FILE_H__
