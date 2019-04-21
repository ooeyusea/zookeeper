#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__
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
	};
}

#endif //__FILE_SYSTEM_H__
