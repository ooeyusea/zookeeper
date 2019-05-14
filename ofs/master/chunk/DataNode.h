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

		inline void SetKey(const std::string& val) { _key = val; }
		inline void SetKey(const char * val) { _key = val; }
		inline void SetKey(std::string&& val) { _key = val; }
		inline const std::string& GetKey() const { return _key; }

		inline bool IsUseAble() const { return !_fault && olib::GetTickCount() - _tick <= CS_BROCK_TIMEOUT; }
		inline void SetFault(bool fault) { _fault = fault; }
		inline void UpdateTick() { _tick = olib::GetTickCount(); }

		inline void SetRemain(int32_t val) { _remain = val; }
		inline int32_t GetRemain() const { return _remain; }

		inline void SetLoad(int32_t val) { _load = val; }
		inline int32_t GetLoad() const { return _load; }

		std::string CalcKey(int64_t id, int64_t lease, int64_t version, int64_t expectVersion);

	protected:
		int32_t _id;
		std::string _host;
		int32_t _port;
		std::string _key;
		int64_t _tick;
		bool _fault = false;
		int32_t _remain;
		int32_t _load;
	};
}

#endif //__DATANODE_H__
