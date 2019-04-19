#ifndef __OFS_MASTER_H__
#define __OFS_MASTER_H__
#include "paxos.h"

namespace ofs {
	class Master {
	public:
		Master();
		virtual ~Master() {}

		bool Start();
	private:
	};
}

#endif //__OFS_MASTER_H__
