#ifndef __OFS_MASTER_H__
#define __OFS_MASTER_H__
#include "hnet.h"
#include "file/FileSystem.h"
#include "rpc/Rpc.h"

namespace ofs {
	class Master {
	public:
		Master();
		virtual ~Master() {}

		bool Start(const std::string& path);

	private:
	};
}

#endif //__OFS_MASTER_H__
