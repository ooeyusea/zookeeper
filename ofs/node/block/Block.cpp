#include "Block.h"
#include "XmlReader.h"
#include "BlockManager.h"
#include "api/OfsChunk.pb.h"

namespace ofs {
	int32_t Block::Read(int32_t offset, int32_t size, std::string& data) {
		if (_info.size < offset + size)
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		LocalFile * file = BlockManager::Instance().GetBlockFile(_info.id);
		if (!file)
			return api::chunk::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;

		
		if (!file->Read(offset, size, data)) {
			file->Release();;
			return api::chunk::ErrorCode::EC_BLOCK_READ_FAILED;
		}

		file->Release();
		return api::chunk::ErrorCode::EC_NONE;
	}

	int32_t Block::Write(int32_t offset, const std::string& data) {
		if (_info.size < offset + data.size())
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		LocalFile * file = BlockManager::Instance().GetBlockFile(_info.id);
		if (!file)
			return api::chunk::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;

		bool ret = file->Write(offset, data.c_str(), data.size());
		file->Release();
		if (!ret)
			return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;

		LocalFile * meta = BlockManager::Instance().GetBlockMetaFile(_info.id);
		if (!meta)
			return api::chunk::ErrorCode::EC_BLOCK_META_FILE_NOT_EXIST;

		ret = meta->Write(0, (const char *)&_info, sizeof(_info));
		meta->Release();
		if (!ret)
			return api::chunk::ErrorCode::EC_BLOCK_WRITE_META_FAILED;

		return api::chunk::ErrorCode::EC_NONE;
	}
}
