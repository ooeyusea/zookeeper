#include "BlockManager.h"

namespace ofs {
	BlockManager::BlockManager() {

	}

	bool BlockManager::Start(const olib::IXmlObject& object) {
		return true;
	}

	LocalFile * BlockManager::GetBlockFile(int32_t blockId) {
		std::lock_guard<hn_mutex> guard(_mutex);

		char file[MAX_PATH];
		snprintf(file, sizeof(file), "%s/%d.block", _blockPath.c_str(), blockId);
		return _cache.Find(file);
	}

	LocalFile * BlockManager::GetBlockMetaFile(int32_t blockId) {
		std::lock_guard<hn_mutex> guard(_mutex);

		char file[MAX_PATH];
		snprintf(file, sizeof(file), "%s/%d_meta.block", _blockPath.c_str(), blockId);
		return _cache.Find(file);
	}
}

