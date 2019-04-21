#ifndef __FILE_H__
#define __FILE_H__
#include "hnet.h"
#include "Node.h"

namespace ofs {
	class File : public Node {
		struct Chunk {
			std::string meta;
			int64_t version;
			int32_t offset;
			int32_t size;

			std::vector<int32_t> places;
		};
	public:
		File() : Node(false) {}
		~File() {}

		virtual void DoNotWantToObject() {}

		void ClearChunk();

	private:
		std::vector<Chunk> _chunks;
	};
}

#endif //__OFS_MASTER_H__
