#include "Block.h"
#include "XmlReader.h"
#include "BlockManager.h"
#include "api/OfsChunk.pb.h"
#include "random_access_file.h"
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
		if (_version == version) {
			if (!_file) {
				std::string path = BlockManager::Instance().GetBlockFile(id);
				_file = new olib::RandomAccessFile(path.c_str(), "w");
			}

			if (_file) {
				auto ret = _file->Write(offset, data.c_str(), data.size());

				if (ret == olib::RandomAccessFileResult::SUCCESS) {
					_dataFlags[offset / BlockManager::Instance().GetBatchSize()] = true;
					return true;
				}
				else {
					hn_error("block {} recover offset {} failed", id, offset);
				}
			}
		}

		return false;
	}

	int32_t Block::Read(int32_t offset, int32_t size, std::string& data) {
		if (_info.size < offset + size) {
			hn_trace("read block {} form offset {}:{} out of range", _info.id, offset, size);
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;
		}

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);

		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover) {
			hn_trace("read block {} but is recovering", _info.id);
			return api::chunk::ErrorCode::EC_BLOCK_IS_RECOVERING;
		}

		data.resize(size);
		auto ret = olib::RandomAccessFile(path.c_str(), "r").Read(offset, (char*)data.c_str(), size);
		if (ret != olib::RandomAccessFileResult::SUCCESS) {
			hn_warn("read block {} from offset {}:{} error {}", _info.id, offset, size, (int8_t)ret);
		}

		switch (ret) {
		case olib::RandomAccessFileResult::SUCCESS: return api::chunk::ErrorCode::EC_NONE;
		case olib::RandomAccessFileResult::FILE_OPEN_FAILED: return api::chunk::ErrorCode::EC_BLOCK_FILE_NOT_EXIST;
		case olib::RandomAccessFileResult::OP_FAILED: return api::chunk::ErrorCode::EC_BLOCK_READ_FAILED;
		case olib::RandomAccessFileResult::OP_INCOMPELETE: return api::chunk::ErrorCode::EC_BLOCK_READ_FAILED;
		case olib::RandomAccessFileResult::OP_OUT_OF_RANGE: return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;
		}

		return api::chunk::ErrorCode::EC_BLOCK_READ_FAILED;
	}

	int32_t Block::Write(int64_t exceptVersion, int64_t newVersion, int32_t offset, const std::string& data, bool strict) {
		if (_info.size < offset + data.size()) {
			hn_trace("write block {} from offset {}:{} but out of range", _info.id, offset, data.size());
			return api::chunk::ErrorCode::EC_BLOCK_OUT_OF_RANGE;
		}

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover) {
			hn_trace("write block {} from offset {}:{} but is recovering", _info.id, offset, data.size());
			return api::chunk::ErrorCode::EC_BLOCK_IS_RECOVERING;
		}

		if (strict) {
			if (_info.version != exceptVersion) {
				hn_warn("write block {} from offset {}:{} but is version is not math {}!={}", _info.id, offset, data.size(), _info.version, exceptVersion);
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
			}
		}
		else {
			if (_info.version < exceptVersion) {
				hn_trace("write block {} from offset {}:{} but is version is not math {}<{}", _info.id, offset, data.size(), _info.version, exceptVersion);
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
			}
		}

		auto ret = olib::RandomAccessFile(path.c_str(), "w").Write(offset, data.c_str(), data.size());
		if (ret != olib::RandomAccessFileResult::SUCCESS) {
			hn_warn("write block {} from offset {}:{} error {}", _info.id, offset, data.size(), (int8_t)ret);
		}
		switch (ret) {
		case olib::RandomAccessFileResult::FILE_OPEN_FAILED: return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;
		case olib::RandomAccessFileResult::OP_FAILED: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		case olib::RandomAccessFileResult::OP_INCOMPELETE: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		}

		if (strict)
			_info.version = newVersion;
		else {
			if (_info.version < newVersion)
				_info.version = newVersion;
			else
				++_info.version;
		}

		ret = olib::RandomAccessFile(metaPath.c_str(), "w").Write(0, (const char*)& _info, sizeof(_info));
		if (ret != olib::RandomAccessFileResult::SUCCESS) {
			hn_warn("write block {} from offset {}:{} write meta error {}", _info.id, offset, data.size(), (int8_t)ret);
		}
		switch (ret) {
		case olib::RandomAccessFileResult::SUCCESS: return api::chunk::ErrorCode::EC_NONE;
		case olib::RandomAccessFileResult::FILE_OPEN_FAILED: return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;
		case olib::RandomAccessFileResult::OP_FAILED: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		case olib::RandomAccessFileResult::OP_INCOMPELETE: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		}
		
		return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
	}

	int32_t Block::Append(int64_t exceptVersion, int64_t newVersion, const std::string& data, bool strict) {
		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_recover) {
			hn_trace("append block {} for size {} but is recovering", _info.id, data.size());
			return api::chunk::ErrorCode::EC_BLOCK_IS_RECOVERING;
		}

		if (strict) {
			if (_info.version != exceptVersion) {
				hn_warn("append block {} for size {} but is version is not math {}!={}", _info.id, data.size(), _info.version, exceptVersion);
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
			}
		}
		else {
			if (_info.version < exceptVersion) {
				hn_trace("append block {} for size {} but is version is not math {}<{}", _info.id, data.size(), _info.version, exceptVersion);
				return api::chunk::ErrorCode::EC_WRITE_BLOCK_VERSION_CHECK_FAILED;
			}
		}

		if (_info.size + data.size() > BlockManager::Instance().GetBlockSize()) {
			hn_warn("append block {} for size {} block has not enougth space", _info.id, data.size());
			return api::chunk::EC_BLOCK_FULL;
		}

		auto ret = olib::RandomAccessFile(path.c_str(), "w").Append(data.c_str(), data.size());
		if (ret != olib::RandomAccessFileResult::SUCCESS) {
			hn_warn("append block {} for size {} error {}", _info.id, data.size(), (int8_t)ret);
		}
		switch (ret) {
		case olib::RandomAccessFileResult::FILE_OPEN_FAILED: return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;
		case olib::RandomAccessFileResult::OP_FAILED: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		case olib::RandomAccessFileResult::OP_INCOMPELETE: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		}

		_info.size += (int32_t)data.size();

		if (strict)
			_info.version = newVersion;
		else {
			if (_info.version < newVersion)
				_info.version = newVersion;
			else
				++_info.version;
		}

		ret = olib::RandomAccessFile(metaPath.c_str(), "w").Write(0, (const char*)& _info, sizeof(_info));
		if (ret != olib::RandomAccessFileResult::SUCCESS) {
			hn_warn("append block {} for size {} write meta error {}", _info.id, data.size(), (int8_t)ret);
		}
		switch (ret) {
		case olib::RandomAccessFileResult::SUCCESS: return api::chunk::ErrorCode::EC_NONE;
		case olib::RandomAccessFileResult::FILE_OPEN_FAILED: return api::chunk::ErrorCode::EC_BLOCK_OPEN_OR_CREATE_FILE_FAILED;
		case olib::RandomAccessFileResult::OP_FAILED: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		case olib::RandomAccessFileResult::OP_INCOMPELETE: return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
		}

		return api::chunk::ErrorCode::EC_BLOCK_WRITE_FAILED;
	}

	void Block::Remove() {
		hn_info("remove block {} size {}", _info.id, _info.size);

		std::string path = BlockManager::Instance().GetBlockFile(_info.id);
		std::string metaPath = BlockManager::Instance().GetBlockMetaFile(_info.id);

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		fs::remove(path);
		fs::remove(metaPath);
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
			Release();
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
			auto ret = olib::RandomAccessFile(metaPath.c_str(), "w").Write(0, (const char*)& _info, sizeof(_info));
			if (ret == olib::RandomAccessFileResult::SUCCESS)
				return true;
		}

		return false;
	}
}
