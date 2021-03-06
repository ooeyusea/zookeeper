#include "FileSystem.h"
#include "file_system.h"
#include "XmlReader.h"
#include "api/OfsMaster.pb.h"
#include "bufferstream.h"
#include <fstream>

#define KB 1024
#define MB (1024 * KB)

namespace ofs {
	bool FileSystem::Start(const olib::IXmlObject& root) {
		_path = root["data"][0]["path"][0].GetAttributeString("val");
		_blockSize = root["data"][0]["block"][0].GetAttributeInt32("size") * MB;
		int64_t interval = root["data"][0]["save"][0].GetAttributeInt64("interval") * SECOND;

		_deleteExpireTime = root["data"][0]["file"][0].GetAttributeInt64("expire") * SECOND;
		int64_t gc = root["data"][0]["file"][0].GetAttributeInt64("gc") * SECOND;

		if (!LoadFromFile(_path))
			return false;

		StartSave(interval);
		StartCleanDeleteFile(gc);
		return true;
	}

	void FileSystem::Flush() {
		SaveToFile(_path);
	}

	bool FileSystem::LoadFromFile(const std::string& path) {
		if (fs::exists(path)) {
			olib::BufferFileStream<> fp(path);
			hyper_net::IArchiver<olib::BufferFileStream<>> ar(fp, 0);
			ar >> _root;

			if (ar.Fail()) {
				hn_error("parse meta data file failed");
				return false;
			}

			_root.SetRoot(true);
			_root.BuildAllFile();
			hn_info("load directory complete");
		}
		else {
			_root.SetOwner("root");
			_root.SetOwnerGroup("root");
			_root.SetAuthority(api::master::AuthorityType::AT_OWNER_READ | api::master::AuthorityType::AT_OWNER_WRITE | api::master::AuthorityType::AT_GROUP_READ | api::master::AuthorityType::AT_OTHER_READ);
			_root.SetCreateTime(olib::GetTimeStamp());
			_root.SetUpdateTime(olib::GetTimeStamp());
			_root.SetRoot(true);

			SaveToFile(path);
			hn_info("not exist directory file");
		}

		return true;
	}

	bool FileSystem::SaveToFile(const std::string& path) {
		std::ofstream out(path);
		hyper_net::OArchiver<std::ofstream> ar(out, 0);

		ar << _root;

		return !ar.Fail();
	}

	void FileSystem::FileGC() {
		_root.StartGC(olib::GetTimeStamp());
	}

	void FileSystem::StartSave(int64_t interval) {
		_saveTimer = new hn_ticker(interval);

		hn_fork[this]{
			for (auto t : *_saveTimer) {
				SaveToFile(_path);
			}
		};
	}

	void FileSystem::StartCleanDeleteFile(int64_t interval) {
		_cleanTimer = new hn_ticker(interval);

		hn_fork[this]{
			for (auto t : *_cleanTimer) {
				FileGC();
			}
		};
	}
}
