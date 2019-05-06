#ifndef __OFS_MASTER_H__
#define __OFS_MASTER_H__
#include "hnet.h"
#include "singleton.h"

namespace ofs {
	class Master : public olib::Singleton<Master> {
	public:
		Master();
		virtual ~Master() {}

		bool Start(const std::string& path);
	};
}

#endif //__OFS_MASTER_H__
