#ifndef __OFS_FILE_SYSTEM_H__
#define __OFS_FILE_SYSTEM_H__
#include "hnet.h"
#include "Directory.h"
#include "singleton.h"

namespace ofs {
	class FileSystem : public olib::Singleton<FileSystem> {
	public:
		FileSystem() {}
		~FileSystem() {}

		bool LoadFromFile(const std::string& path);
		bool SaveToFile(const std::string& path);

		Directory& Root() { return _root; }

	private:
		Directory _root;

		std::string _path;
	};
}

#endif //__OFS_FILE_SYSTEM_H__
