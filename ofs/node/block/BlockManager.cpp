#include "BlockManager.h"

namespace ofs {
	BlockManager::BlockManager() {

	}

	bool BlockManager::Start(const olib::IXmlObject& object) {
		_blockPath = object["data"][0]["path"][0].GetAttributeString("val");
		return true;
	}

	std::string BlockManager::GetBlockFile(int32_t blockId) {
		std::lock_guard<hn_mutex> guard(_mutex);

		char file[MAX_PATH];
		snprintf(file, sizeof(file), "%s/%d.block", _blockPath.c_str(), blockId);
		return file;
	}

	std::string BlockManager::GetBlockMetaFile(int32_t blockId) {
		std::lock_guard<hn_mutex> guard(_mutex);

		char file[MAX_PATH];
		snprintf(file, sizeof(file), "%s/%d_meta.block", _blockPath.c_str(), blockId);
		return file;
	}
}
