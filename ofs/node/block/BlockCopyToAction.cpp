#include "BlockCopyToAction.h"
#include "BlockManager.h"
#include "file/LocalFile.h"
#include "time_helper.h"
#include "proto/Chunk.pb.h"
#include "node/NodeService.h"
#include "file_system.h"
#include <fstream>

#ifdef WIN32
#define STACK_DYNAMIC_ARR(data, size) char * data = (char *)alloca(size)
#else
#define STACK_DYNAMIC_ARR(data, size) char data[size]
#endif 

namespace ofs {
	void BlockCopyToAction::Start(int64_t id, int64_t version, int32_t size, int32_t copyTo) {
		std::string path = BlockManager::Instance().GetBlockFile(id);
		int32_t fileSize = (int32_t)fs::file_size(path);
		if (fileSize != size)
			return;

		if ((size % BlockManager::Instance().GetBatchSize()) != 0)
			return;

		c2m::StartRecoverBlock resize;
		resize.set_blockid(id);
		resize.set_version(version);
		resize.set_size(size);

		NodeService::Instance().Send(copyTo, resize);

		std::ifstream input(path, std::ios::binary);
		if (!input)
			return;

		STACK_DYNAMIC_ARR(buf, BlockManager::Instance().GetBatchSize());
		int32_t offset = 0;
		while (input) {
			input.read(buf, BlockManager::Instance().GetBatchSize());
			if (input.gcount() == 0)
				break;

			c2m::RecoverBlockData data;
			data.set_blockid(id);
			data.set_offset(offset);
			data.set_version(version);
			data.set_allocated_data(new std::string(buf, input.gcount()));

			NodeService::Instance().Send(copyTo, data);

			offset += input.gcount();
		}

		if (offset != size)
			return;

		c2m::RecoverBlockComplete complete;
		complete.set_blockid(id);
		complete.set_version(version);

		NodeService::Instance().Send(copyTo, complete);
	}
}
