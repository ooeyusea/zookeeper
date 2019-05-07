#ifndef __CHUNKSERVER_H__
#define __CHUNKSERVER_H__
#include "hnet.h"

namespace ofs {
	class ChunkServer {
	public:
		ChunkServer() {}
		~ChunkServer() {}

		inline void SetHost(const std::string& val) { _host = val; }
		inline void SetHost(std::string&& val) { _host = val; }
		inline const std::string& GetHost() const { return _host; }

		inline void SetPort(int32_t val) { _port = val; }
		inline int32_t GetPort() const { return _port; }

		bool BusyThen(const ChunkServer& rhs) const;

	public:
		std::string _host;
		int32_t _port;
	};
}

#endif //__CHUNKSERVER_H__
