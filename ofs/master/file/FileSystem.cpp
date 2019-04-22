#include "FileSystem.h"
#include "file_system.h"
#include "XmlReader.h"
#include "api/OfsMaster.pb.h"

namespace ofs {
	bool FileSystem::Start(const olib::IXmlObject& root) {
		if (!LoadFromFile(root["data"][0]["path"][0].GetAttributeString("val")))
			return false;
		return true;
	}

	bool FileSystem::LoadFromFile(const std::string& path) {
		_path = path;

		if (fs::exists(path)) {
			olib::XmlReader conf;
			if (!conf.LoadXml(path.c_str())) {
				hn_error("load directory file failed");
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
		return false;
	}

}
