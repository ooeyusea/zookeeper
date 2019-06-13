#ifndef __DATA_CLUSTER_H__
#define __DATA_CLUSTER_H__
#include "hnet.h"
#include "XmlReader.h"
#include "DataNode.h"

namespace ofs {
	struct IDataCluster {
		~IDataCluster() {}

		virtual bool Start(const olib::IXmlObject& root) = 0;
		virtual DataNode * Get(int32_t id) = 0;
		virtual DataNode * Register(int32_t id, int32_t rack, int32_t dc, const std::string& extend) = 0;
		virtual std::vector<DataNode*> Distribute(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except) = 0;
		virtual std::vector<DataNode*> SelectUnnecessary(std::vector<DataNode*>&& old) = 0;
	};
}

#endif //__DATA_CLUSTER_H__
