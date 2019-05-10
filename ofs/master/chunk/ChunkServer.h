#ifndef __CHUNKSERVER_H__
#define __CHUNKSERVER_H__
#include "hnet.h"

namespace ofs {
	enum ChunkServerStatus {
		CSS_BROKEN = 0,
		CSS_READY,
	};

	class ChunkServer {
	public:
		ChunkServer() {}
		~ChunkServer() {}

		inline void SetId(int32_t val) { _id = val; }
		inline int32_t GetId() const { return _id; }

		inline void SetHost(const std::string& val) { _host = val; }
		inline void SetHost(const char * val) { _host = val; }
		inline void SetHost(std::string&& val) { _host = val; }
		inline const std::string& GetHost() const { return _host; }

		inline void SetPort(int32_t val) { _port = val; }
		inline int32_t GetPort() const { return _port; }

		inline void SetStatus(int8_t val) { _status = val; }
		inline int8_t GetStatus() const { return _status; }

		inline void SetKey(const std::string& val) { _key = val; }
		inline void SetKey(const char * val) { _key = val; }
		inline void SetKey(std::string&& val) { _key = val; }
		inline const std::string& GetKey() const { return _key; }

		std::string CalcKey(int64_t id, int64_t lease, int64_t version, int64_t expectVersion);

	public:
		int32_t _id;
		std::string _host;
		int32_t _port;
		int8_t _status = ChunkServerStatus::CSS_BROKEN;
		std::string _key;
	};
}

#endif //__CHUNKSERVER_H__
