#include "URLGraph.h"
#include "URLReader.h"

void UrlGraph::Search(const char * entry, int32_t limit, int32_t processorCount) {
	hn_channel<UrlEvent, -1> evts;
	evts << UrlEvent{entry, UES_OPEN};

	hn_channel<std::string, 20> waitDoing;
	for (int32_t i = 0; i < processorCount; ++i) {
		hn_fork hn_stack(64 * 1024 * 1024) [evts, waitDoing, this]{
			try {
				while (true) {
					std::string url;
					waitDoing >> url;

					auto request = UrlReader::Instance().Get(rand(), url.c_str());
					if (!_outputCookie.empty())
						request.SetCookie(_outputCookie.c_str());

					request.Commit();
					if (!request.Bad()) {
						auto urls = _processFn(request.GetContent());
						for (auto& newUrl : urls)
							evts << UrlEvent{ newUrl, UES_OPEN };
					}

					evts << UrlEvent{ url, UES_DONE };
				}
			}
			catch (hn_channel_close_exception& e) {
				
			}
		};
	}

	std::unordered_set<std::string> pendings;
	std::unordered_set<std::string> dones;
	try {
		while (true) {
			UrlEvent evt;
			evts >> evt;

			if (evt.status == UES_OPEN) {
				if (limit == 0 || pendings.size() < limit) {
					auto itr = pendings.find(evt.url);
					if (itr == pendings.end()) {
						waitDoing << evt.url;
						pendings.insert(std::move(evt.url));
					}
				}
			}
			else if (evt.status == UES_DONE) {
				dones.insert(std::move(evt.url));
				if (dones.size() == pendings.size()) {
					evts.Close();
					waitDoing.Close();
					break;
				}
			}
		}
	}
	catch (hn_channel_close_exception& e) {

	}
}

