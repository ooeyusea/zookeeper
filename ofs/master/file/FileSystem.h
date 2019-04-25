#ifndef __OFS_FILE_SYSTEM_H__
#define __OFS_FILE_SYSTEM_H__
#include "hnet.h"
#include "Directory.h"
#include "singleton.h"
#include "XmlReader.h"

namespace ofs {
	class FileSystem : public olib::Singleton<FileSystem> {
	public:
		FileSystem() {}
		~FileSystem() {}

		bool Start(const olib::IXmlObject& root);
		void Flush();

		Directory& Root() { return _root; }

	private:
		bool LoadFromFile(const std::string& path);
		bool SaveToFile(const std::string& path);

	private:
		Directory _root;

		std::string _path;
	};
}

#endif //__OFS_FILE_SYSTEM_H__
