#ifndef __OFS_NODE_H__
#define __OFS_NODE_H__
#include "hnet.h"
#include "singleton.h"

namespace ofs {
	class Node : public olib::Singleton<Node> {
	public:
		Node();
		virtual ~Node() {}

		bool Start(const std::string& path);
	};
}

#endif //__OFS_NODE_H__
