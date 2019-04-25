#include "FileSystem.h"
#include "file_system.h"
#include "XmlReader.h"
#include "api/OfsMaster.pb.h"
#include "bufferstream.h"
#include <fstream>

namespace ofs {
	bool FileSystem::Start(const olib::IXmlObject& root) {
		_path = root["data"][0]["path"][0].GetAttributeString("val");
		if (!LoadFromFile(_path))
			return false;
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

			hn_info("load directory complete");
		}
		else {
			_root.SetOwner("root");
			_root.SetOwnerGroup("root");
			_root.SetAuthority(api::AuthorityType::AT_OWNER_READ | api::AuthorityType::AT_OWNER_WRITE | api::AuthorityType::AT_GROUP_READ | api::AuthorityType::AT_OTHER_READ);

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

}
