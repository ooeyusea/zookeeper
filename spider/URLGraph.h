#ifndef __URLGRAPH_H__
#define __URLGRAPH_H__
#include "hnet.h"

class UrlGraph { 
	struct UrlTask {
		std::string url;
		int32_t count = 0;

		bool operator<(const UrlTask& rhs) const {
			return count < rhs.count;
		}
	};

	enum {
		UES_OPEN = 0,
		UES_DONE,
	};

	struct UrlEvent {
		std::string url;
		int8_t status = 0;
	};

public:
	~UrlGraph() {}

	static UrlGraph& Instance() {
		static UrlGraph g_instance;
		return g_instance;
	}

	inline UrlGraph& SetProccessor(const std::function<std::list<std::string>(const std::string&)>& fn) {
		_processFn = fn;
		return *this;
	}

	inline UrlGraph& SetCookie(std::string&& cookie) {
		_outputCookie = cookie;
		return *this;
	}

	void Search(const char * entry, int32_t limit, int32_t processorCount);

private:
	UrlGraph() {}

	std::function<std::list<std::string>(const std::string&)> _processFn;

	std::string _outputCookie;
};

#endif //__URLGRAPH_H__
