#ifndef __OFS_FILE_SYSTEM_H__
#define __OFS_FILE_SYSTEM_H__
#include "hnet.h"
#include "Directory.h"
#include "singleton.h"
#include "XmlReader.h"
#include "uuid.h"

namespace ofs {
	class FileSystem : public olib::Singleton<FileSystem> {
	public:
		FileSystem() {}
		~FileSystem() {}

		bool Start(const olib::IXmlObject& root);
		void Flush();

		inline Directory& Root() { return _root; }

		inline int32_t GetBlockCount() { return _blockCount; }
		inline int32_t GetBlockSize() { return _blockSize; }

		inline void AddFile(File* file) {
			_files.insert(std::make_pair(file->GetUUID(), file));
		}

	private:
		bool LoadFromFile(const std::string& path);
		bool SaveToFile(const std::string& path);

	private:
		Directory _root;

		std::string _path;
		std::unordered_map<olib::uuid::UUID, File*> _files;

		int32_t _blockCount;
		int32_t _blockSize;
	};
}

#endif //__OFS_FILE_SYSTEM_H__
