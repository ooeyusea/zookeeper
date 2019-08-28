#include "random_access_file.h"

#define MAX_DISK_TIMES 5
#define OPEN_MODE 0644

namespace olib {
	RandomAccessFile::RandomAccessFile(const char* path, const char* mode) 
		: _path(path)
		, _mode(mode) {
	}

	RandomAccessFile::~RandomAccessFile() {
#ifdef WIN32
		if (_fd != INVALID_HANDLE_VALUE)
			CloseHandle(_fd);
#else
		if (_fd > 0)
			close(_fd);
#endif
	}

	RandomAccessFileResult RandomAccessFile::Read(int64_t offset, char* buf, int64_t size) {
		int64_t readOffset = 0;
		int64_t left = size;

		int32_t tryTime = 0;
		while (left > 0) {
			if (tryTime++ >= MAX_DISK_TIMES)
				break;

			if (!IsOpened() && !Open(nullptr))
				return RandomAccessFileResult::FILE_OPEN_FAILED;

#ifdef WIN32
			LARGE_INTEGER distanceToMove;
			distanceToMove.QuadPart = offset + readOffset;
			SetFilePointerEx(_fd, distanceToMove, NULL, FILE_BEGIN);

			DWORD len = 0;
			if (!ReadFile(_fd, buf + readOffset, left, &len, nullptr)) {
				auto err = GetLastError();
				if (err == ERROR_IO_PENDING)
					continue;

				hn_warn("read file {} at offset {}[{}] for size {} failed {}", _path, offset, readOffset, size, err);
				return RandomAccessFileResult::OP_FAILED;
			}
#else
			int64_t len = pread64(_fd, buf + readOffset, left, offset + readOffset);
			if (len < 0) {
				if (EINTR == errno || EAGAIN == errno)
					continue; /* call pread64() again */
				else if (EBADF == errno) {
					_fd = -1;
					continue;
				}
				else {
					hn_warn("read file {} at offset {}[{}] for size {} failed {}", _path, offset, readOffset, size, errno);
					return RandomAccessFileResult::OP_NOT_COMPELETE;
				}
			}
#endif
			if (0 == len) {
				if (left > 0)
					return RandomAccessFileResult::OP_OUT_OF_RANGE;
				break;
			}

			left -= len;
			readOffset += len;
		}

		if (left > 0)
			return RandomAccessFileResult::OP_INCOMPELETE;

		return RandomAccessFileResult::SUCCESS;
    }

	RandomAccessFileResult RandomAccessFile::Write(int64_t offset, const char* buf, int64_t size) {
		int64_t writeOffset = 0;
		int64_t left = size;

		int32_t tryTime = 0;
		while (left > 0) {
			if (tryTime++ >= MAX_DISK_TIMES)
				break;

			if (!IsOpened() && !Open(nullptr))
				return RandomAccessFileResult::FILE_OPEN_FAILED;

#ifdef WIN32
			LARGE_INTEGER distanceToMove;
			distanceToMove.QuadPart = offset + writeOffset;
			SetFilePointerEx(_fd, distanceToMove, NULL, FILE_BEGIN);

			DWORD len = 0;
			if (!WriteFile(_fd, buf + writeOffset, left, &len, nullptr)) {
				auto err = GetLastError();
				if (err == ERROR_IO_PENDING)
					continue;

				hn_error("write file {} at offset {}[{}] for size {} failed {}", _path, offset, writeOffset, size, err);
				return RandomAccessFileResult::OP_FAILED;
			}
#else
			int64_t len = pwrite64(_fd, buf + writeOffset, left, offset + writeOffset);
			if (len < 0) {
				if (EINTR == errno || EAGAIN == errno)
					continue; /* call pread64() again */
				else if (EBADF == errno) {
					_fd = -1;
					continue;
				}
				else {
					hn_warn("write file {} at offset {}[{}] for size {} failed {}", _path, offset, writeOffset, size, errno);
					return RandomAccessFileResult::OP_NOT_COMPELETE;
				}
			}
#endif
			if (0 == len)
				break;

			left -= len;
			writeOffset += len;
		}

		if (left > 0)
			return RandomAccessFileResult::OP_INCOMPELETE;

		return RandomAccessFileResult::SUCCESS;
	}

	RandomAccessFileResult RandomAccessFile::Append(const char* buf, int64_t size) {
		int64_t writeOffset = 0;
		int64_t left = size;
		int32_t offset = 0;

		int32_t tryTime = 0;
		while (left > 0) {
			if (tryTime++ >= MAX_DISK_TIMES)
				break;

			if (!IsOpened() && !Open((tryTime == 1) ? &offset : nullptr))
				return RandomAccessFileResult::FILE_OPEN_FAILED;

#ifdef WIN32
			LARGE_INTEGER distanceToMove;
			distanceToMove.QuadPart = offset + writeOffset;
			SetFilePointerEx(_fd, distanceToMove, NULL, FILE_BEGIN);

			DWORD len = 0;
			if (!WriteFile(_fd, buf + writeOffset, left, &len, nullptr)) {
				auto err = GetLastError();
				if (err == ERROR_IO_PENDING)
					continue;

				hn_warn("write file {} at offset {}[{}] for size {} failed {}", _path, offset, writeOffset, size, err);
				return RandomAccessFileResult::OP_FAILED;
			}
#else
			int64_t len = pwrite64(_fd, buf + writeOffset, left, offset + writeOffset);
			if (len < 0) {
				if (EINTR == errno || EAGAIN == errno)
					continue; /* call pread64() again */
				else if (EBADF == errno) {
					_fd = -1;
					continue;
				}
				else {
					hn_warn("write file {} at offset {}[{}] for size {} failed {}", _path, offset, writeOffset, size, errno);
					return RandomAccessFileResult::OP_NOT_COMPELETE;
				}
			}
#endif
			if (0 == len)
				break;

			left -= len;
			writeOffset += len;
		}

		if (left > 0)
			return RandomAccessFileResult::OP_INCOMPELETE;

		return RandomAccessFileResult::SUCCESS;
	}

	bool RandomAccessFile::Open(int32_t * fileSize) {
#ifdef WIN32
		DWORD dwDesiredAccess = 0;
		if (_mode.find("r") != std::string::npos)
			dwDesiredAccess |= GENERIC_READ;

		if (_mode.find("w") != std::string::npos)
			dwDesiredAccess |= GENERIC_WRITE;

		_fd = CreateFile(_path.c_str(), dwDesiredAccess, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, nullptr);
		if (_fd != INVALID_HANDLE_VALUE) {
			if (nullptr != fileSize) {
				LARGE_INTEGER size;
				::GetFileSizeEx(_fd, &size);

				*fileSize = size.QuadPart;
			}
			return true;
		}
#else
		bool read = (_mode.find("r") != std::string::npos);
		bool write = (_mode.find("w") != std::string::npos);
		
		int32_t flag = 0;
		if (read && write)
			flag = O_RDWR;
		else if (read)
			flag = O_RDONLY;
		else if (write)
			flag = O_WRONLY;

		_fd = ::open(_path.c_str(), flag, OPEN_MODE);
		if (_fd > 0) {
			if (nullptr != fileSize) {
				stat stbuf;
				if ((fstat(_fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
					close(_fd);
					_fd = -1;
					return false;
				}

				*fileSize = stbuf.st_size;
			}
			return true;
		}
#endif
		return false;
	}
}
