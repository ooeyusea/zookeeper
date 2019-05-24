#include "BlockManager.h"
#include "node/NodeService.h"
#include "proto/Chunk.pb.h"

namespace ofs {
	BlockManager::BlockManager() {

	}

	bool BlockManager::Start(const olib::IXmlObject& object) {
		_blockPath = object["data"][0]["path"][0].GetAttributeString("val");
		int64_t interval = object["data"][0]["report"][0].GetAttributeInt64("interval");
		
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
		}
		lock.unlock();

		NodeService::Instance().ReportBlocks(report);
	}

	void BlockManager::StartBlockReportHearbeat(int64_t interval) {
		_reportTimer = new hn_ticker(interval);

		hn_fork[this]{
			for (auto t : *_reportTimer) {
				Report();
			}
		};
	}
}
