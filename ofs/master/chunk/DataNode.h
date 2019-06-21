#ifndef __DATANODE_H__
#define __DATANODE_H__
#include "hnet.h"
#include "time_helper.h"

namespace ofs {
	#define CS_BROCK_TIMEOUT HOUR

	class DataNode {
	public:
		DataNode(int32_t id) : _id(id) {}
		~DataNode() {}

		inline int32_t GetId() const { return _id; }

		inline void SetHost(const std::string& val) { _host = val; }
		inline void SetHost(const char * val) { _host = val; }
		inline void SetHost(std::string&& val) { _host = val; }
		inline const std::string& GetHost() const { return _host; }

		inline void SetPort(int32_t val) { _port = val; }
		inline int32_t GetPort() const { return _port; }

		inline void SetClusterHost(const std::string& val) { _clusterHost = val; }
		inline void SetClusterHost(const char* val) { _clusterHost = val; }
		inline void SetClusterHost(std::string&& val) { _clusterHost = val; }
		inline const std::string& GetClusterHost() const { return _clusterHost; }

		inline void SetClusterPort(int32_t val) { _clusterPort = val; }
		inline int32_t GetClusterPort() const { return _clusterPort; }

		inline void SetKey(const std::string& val) { _key = val; }
		inline void SetKey(const char * val) { _key = val; }
		inline void SetKey(std::string&& val) { _key = val; }
		inline const std::string& GetKey() const { return _key; }

		inline bool IsUseAble() const { return !_fault && olib::GetTickCount() - _tick <= CS_BROCK_TIMEOUT; }
		inline void SetFault(bool fault) { _fault = fault; }
		inline void UpdateTick() { _tick = olib::GetTickCount(); }

		inline void SetCpu(int32_t val) { _cpu = val; }
		inline int32_t GetCpu() const { return _cpu; }

		inline void SetRss(int32_t val) { _rss = val; }
		inline int32_t GetRss() const { return _rss; }

		inline void SetVss(int32_t val) { _vss = val; }
		inline int32_t GetVss() const { return _vss; }

		inline void SetDisk(int32_t val) { _disk = val; }
		inline int32_t GetDisk() const { return _disk; }

		std::string CalcKey(int64_t id, int64_t lease, int64_t version, int64_t expectVersion);

	protected:
		int32_t _id = 0;
		std::string _host;
		int32_t _port = 0;
		std::string _clusterHost;
		int32_t _clusterPort = 0;
		std::string _key;
		int64_t _tick = 0;

		bool _fault = false;
		int32_t _cpu = 0;
		int32_t _rss = 0;
		int32_t _vss = 0;
		int32_t _disk = 0;
	};
}

#endif //__DATANODE_H__
