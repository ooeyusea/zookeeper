#include "Block.h"
#include "XmlReader.h"
#include "BlockManager.h"
#include "api/OfsChunk.pb.h"
#include "file/LocalFile.h"
#include "time_helper.h"
#include "BlockCopyToAction.h"
#include "file_system.h"

namespace ofs {
	Block::RecoverController::RecoverController(int64_t version, int32_t size) : _version(version), _size(size) {
		_dataFlags.resize(size / BlockManager::Instance().GetBatchSize(), false);
	}

	bool Block::RecoverController::Resize(int64_t version, int32_t size) {
		if (_version < version) {
			_version = version;
			_size = size;

			_dataFlags.clear();
			_dataFlags.resize(size / BlockManager::Instance().GetBatchSize(), false);

			return true;
		}

		return false;
	}

	bool Block::RecoverController::Recover(int64_t id, int64_t version, int32_t offset, const std::string& data) {
		if (_version != version) {
			if (!_output) {
				std::string path = BlockManager::Instance().GetBlockFile(id);
				_output.open(path, std::ios::app | std::ios::binary);
			}

			if (_output) {
				_output.seekp(offset);
				_output.write(data.c_str(), data.size());

				if (_output) {
					_dataFlags[offset / BlockManager::Instance().GetBatchSize()] = true;
					return true;
				}
			}
		}

		return false;
	}

	int32_t Block::Read(int32_t offset, int32_t size, std::string& data) {
		if (_info.size < offset + size)
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);

		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover)
			return api::chunk::ErrorCode::EC_BLOCK_IS_RECOVERING;

		LocalFile file(std::move(path));
		return file.Read(offset, size, data);
	}

	int32_t Block::Write(int64_t exceptVersion, int64_t newVersion, int32_t offset, const std::string& data, bool strict) {
		if (_info.size < offset + data.size())
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover)
			return api::chunk::ErrorCode::EC_BLOCK_IS_RECOVERING;

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
		if (_recover)
			return api::chunk::ErrorCode::EC_BLOCK_IS_RECOVERING;

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

	void Block::StartRecover(int64_t version, int64_t lease, int32_t copyTo) {
		if (_fault)
			return;

		Acquire();
		hn_fork[this, version, lease, copyTo]{
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			if (_info.version < version) {
				hn_error("block {} try to recover to {}, but version mismatch {} <-> {}", _info.id, copyTo, _info.version, version);
				return;
			}

			int64_t now = olib::GetTimeStamp();
			if (now >= lease) {
				hn_error("block {} try to recover to {}, but lease is passout", _info.id, copyTo);
				return;
			}

			BlockCopyToAction().Start(_info.id, _info.version, _info.size, copyTo);
		};
	}

	void Block::Resize(int64_t version, int32_t size) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_info.version >= version)
			return;

		if (!_recover)
			_recover = new RecoverController(version, size);
		else {
			if (!_recover->Resize(version, size))
				return;
		}

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		if (!fs::exists(path)) {
			std::ofstream out(path, std::ios::binary);
			out.write("c", 1);
			out.flush();
			out.close();
		}
		fs::resize_file(path, size);
	}

	void Block::RecoverData(int64_t version, int32_t offset, const std::string& data) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover)
			_recover->Recover(_info.id, version, offset, data);
	}

	bool Block::CompleteRecover(int64_t version) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover && _recover->CheckIsComplete(version)) {
			_info.version = version;
			_info.size = _recover->GetSize();

			delete _recover;
			_recover = nullptr;

			std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);
			LocalFile meta(metaPath);
			if (meta.Write(0, (const char*)& _info, sizeof(_info)) != api::chunk::ErrorCode::EC_NONE) {
				return true;
			}
		}

		return false;
	}
}
