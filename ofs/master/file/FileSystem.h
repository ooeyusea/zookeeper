#ifndef __OFS_FILE_SYSTEM_H__
#define __OFS_FILE_SYSTEM_H__
#include "hnet.h"
#include "Directory.h"
#include "singleton.h"
#include "XmlReader.h"
#include "OfsId.h"

namespace ofs {
	class FileSystem : public olib::Singleton<FileSystem> {
	public:
		FileSystem() : _mutex(true) {}
		~FileSystem() {}

		bool Start(const olib::IXmlObject& root);
		void Flush();

		inline Directory& Root() { return _root; }

		inline int32_t GetBlockSize() { return _blockSize; }

		inline void AddFile(File* file) {
			std::lock_guard<hn_shared_mutex> guard(_mutex);
			_files.insert(std::make_pair(file->GetId(), file));
		}

		inline File * GetFile(int64_t key) {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			auto itr = _files.find(key);
			if (itr != _files.end())
				return itr->second;
			return nullptr;
		}

		inline int32_t CalcBlockCount(int32_t size) const {
			return (size / _blockSize) + (size % _blockSize == 0) ? 0 : 1;
		}

		inline int32_t CalcAppendBlock(int32_t size) const {
			return (size / _blockSize) + 1;
		}

	private:
		bool LoadFromFile(const std::string& path);
		bool SaveToFile(const std::string& path);

	private:
		void StartSave(int64_t interval);

	private:
		hn_shared_mutex _mutex;

		Directory _root;

		std::string _path;
		std::unordered_map<int64_t, File*> _files;

		int32_t _blockSize;
		hn_ticker* _saveTimer = nullptr;
	};
}

#endif //__OFS_FILE_SYSTEM_H__
