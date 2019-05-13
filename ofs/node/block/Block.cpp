#include "Block.h"
#include "XmlReader.h"
#include "BlockManager.h"

namespace ofs {
	int32_t Block::Read(int32_t offset, int32_t size, std::string& data) {
		LocalFile * file = BlockMangager::Instance().GetBlockFile(_id);
		if (!file)
			return api::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;

		return file->Read(offset, size, data);
	}

	int32_t Block::Write(int32_t offset, const std::string& data) {
		LocalFile* file = BlockMangager::Instance().GetBlockFile(_id);
		if (!file)
			return api::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;



		int32_t ret = file->Write(offset, data);
		if (ret != 0)
			return ret;

		LocalFile* meta = BlockMangager::Instance().GetBlockMetaFile(_id);
		if (!meta)
			return api::ErrorCode::EC_BLOCK_META_FILE_NOT_EXIST;

		return meta->WriteMeta(_id, ++_version);
	}
}
