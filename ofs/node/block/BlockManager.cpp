#include "BlockManager.h"
#include "node/NodeService.h"
#include "proto/Chunk.pb.h"
#include "time_helper.h"
#include "file_system.h"
#include <fstream>

#define KB 1024
#define MB (1024 * KB)

namespace ofs {
	BlockManager::BlockManager() {

	}

	bool BlockManager::Start(const olib::IXmlObject& object) {
		_blockPath = object["data"][0]["path"][0].GetAttributeString("val");
		hn_info("block path : {}", _blockPath);

		_batchSize = object["data"][0]["block"][0].GetAttributeInt32("batch") * KB;
		_blockSize = object["data"][0]["block"][0].GetAttributeInt32("size") * MB;

		hn_info("block size is {}, batch size is {}", _batchSize, _blockSize);

		if (!ScanBlock())
			return false;

		int64_t interval = object["data"][0]["report"][0].GetAttributeInt64("interval") * SECOND;
		StartBlockReportHearbeat(interval);
		return true;
	}

	void BlockManager::Clean(int64_t blockId) {
		std::unique_lock<hn_mutex> lock(_mutex);
		auto itr = _blocks.find(blockId);
		if (itr != _blocks.end()) {
			if (itr->second->IsUsed())
				return;
			
			Block * block = itr->second;
			_blocks.erase(itr);

			block->Remove();
			lock.unlock();

			delete block;

			NodeService::Instance().ReportClean(blockId);
		}
	}

	void BlockManager::Report() {
		c2m::ReportBlock report;

		std::unique_lock<hn_mutex> lock(_mutex);
		for (auto itr = _blocks.begin(); itr != _blocks.end(); ++itr) {
			auto status = report.add_blocks();
			status->set_id(itr->second->GetId());
			status->set_version(itr->second->GetVersion());
			status->set_size(itr->second->GetSize());
			status->set_fault(false);
		}
		lock.unlock();

		NodeService::Instance().ReportBlocks(report);
	}

	bool BlockManager::ScanBlock() {
		olib::FileFinder().Search(_blockPath + "/*_meta.block", [this](const fs::path file) {
			int64_t id = ReadIdFrom(file.stem().string());

			std::ifstream input(file.string(), std::ios::binary);
			if (input.bad()) {
				return;
			}
			
			BlockInfo info;
			input.read((char*)&info, sizeof(info));
			if (input.gcount() != sizeof(info)) {
				return;
			}

			if (info.id != id) {
				hn_error("meta id {} do not match {} in file {}", id, info.id, file.string());
				return;
			}

			int32_t realSize = (int32_t)fs::file_size(GetBlockFile(id));
			if (realSize > info.size) {
				hn_error("meta id {} size {} do not match {}, truncate block file", id, info.size, realSize);
				fs::resize_file(GetBlockFile(id), info.size);
			}
			else if (realSize < info.size) {
				hn_error("meta id {} size {} do not match {}", id, info.size, realSize);
				return;
			}

			Block * block = new Block(id);
			block->SetVersion(info.version);
			block->SetSize(info.size);
			_blocks[id] = block;

			hn_info("load block {}, version {}, size {}", info.id, info.version, info.size);
		});

		hn_info("scan block count : {}", _blocks.size());
		return true;
	}

	void BlockManager::StartBlockReportHearbeat(int64_t interval) {
		_reportTimer = new hn_ticker(interval);
		hn_info("start block report service for interval {}", interval);

		hn_fork[this]{
			for (auto t : *_reportTimer) {
				Report();
			}
		};
	}

	int64_t BlockManager::ReadIdFrom(const std::string& meta) {
		char * endPtr = nullptr;
		return strtoll(meta.c_str(), &endPtr, 10);
	}
}
