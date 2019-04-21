#include "FileSystem.h"

namespace ofs {
	bool FileSystem::LoadFromFile(const std::string& path) {
		_root.SetOwner("root");
		_root.SetOwnerGroup("root");
		_root.SetAuthority();


		return true;
	}

	bool FileSystem::SaveToFile(const std::string& path) {
		return false;
	}

}
