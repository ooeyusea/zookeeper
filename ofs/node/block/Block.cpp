#include "Block.h"
#include "XmlReader.h"
#include "BlockManager.h"
#include "api/OfsChunk.pb.h"
#include "file/LocalFile.h"

namespace ofs {
	int32_t Block::Read(int32_t offset, int32_t size, std::string& data) {
		if (_info.size < offset + size)
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);

		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

		LocalFile file(std::move(path));
		return file.Read(offset, size, data);
	}

	int32_t Block::Write(int32_t offset, const std::string& data) {
		if (_info.size < offset + data.size())
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);

		LocalFile file(std::move(path));
		int32_t ret = file.Write(offset, data.c_str(), data.size());
		if (ret != api::chunk::EC_NONE)
			return ret;

		LocalFile meta(metaPath);
		return meta.Write(0, (const char *)&_info, sizeof(_info));
	}
}
