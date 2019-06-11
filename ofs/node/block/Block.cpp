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

	int32_t Block::Write(int64_t exceptVersion, int64_t newVersion, int32_t offset, const std::string& data, bool strict) {
		if (_info.size < offset + data.size())
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (strict) {
			if (_info.version != exceptVersion)
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
		}
		else {
			if (_info.version < exceptVersion)
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
		}

		LocalFile file(std::move(path));
		int32_t ret = file.Write(offset, data.c_str(), (int32_t)data.size());
		if (ret != api::chunk::EC_NONE)
			return ret;

		if (strict)
			_info.version = newVersion;
		else {
			if (_info.version < newVersion)
				_info.version = newVersion;
			else
				++_info.version;
		}

		LocalFile meta(metaPath);
		return meta.Write(0, (const char *)&_info, sizeof(_info));
	}

	int32_t Block::Append(int64_t exceptVersion, int64_t newVersion, const std::string& data, bool strict) {
		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (strict) {
			if (_info.version != exceptVersion)
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
		}
		else {
			if (_info.version < exceptVersion)
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
		}

		if (_info.size + data.size() > BlockManager::Instance().GetBlockSize())
			return api::chunk::EC_BLOCK_FULL;

		LocalFile file(std::move(path));
		int32_t ret = file.Append(data.c_str(), (int32_t)data.size());
		if (ret != api::chunk::EC_NONE)
			return ret;

		_info.size += (int32_t)data.size();

		if (strict)
			_info.version = newVersion;
		else {
			if (_info.version < newVersion)
				_info.version = newVersion;
			else
				++_info.version;
		}

		LocalFile meta(metaPath);
		return meta.Write(0, (const char *)&_info, sizeof(_info));
	}

	void Block::Remove() {
		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		LocalFile file(std::move(path));
		file.Remove();

		LocalFile meta(metaPath);
		meta.Remove();
	}
}
