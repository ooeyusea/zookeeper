#ifndef __LOCAL_FILE_H__
#define __LOCAL_FILE_H__
#include "hnet.h"

namespace ofs {
	class LocalFile {
	public:
		LocalFile(const std::string& path) : _path(path){}
		LocalFile(std::string&& path) : _path(path) {}
		~LocalFile() {}

		int32_t Read(int32_t offset, int32_t size, std::string& data);
		int32_t Write(int32_t offset, const char * data, int32_t size);
		int32_t Append(const char * data, int32_t size);
		void Remove();

	private:
		std::string _path;
	};
}

#endif //__LOCAL_FILE_H__
