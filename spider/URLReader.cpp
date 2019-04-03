#include "URLReader.h"
#include "XmlReader.h"

size_t UrlReader::UrlRequest::WriteData(void *ptr, size_t size, size_t nmemb, void *stream) {
	std::string& data = *(std::string*)stream;
	data.append((const char*)ptr, size * nmemb);

	return size * nmemb;
}

void UrlReader::UrlRequest::OnExecute() {
	curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
	_errCode = curl_easy_perform(_curl);
}

bool UrlReader::Start(int32_t threadCount) {

	_threadCount = threadCount;
	hn_info("url reader thread count {}", _threadCount);

	_queue = hn_create_async(_threadCount, true, [this](void * src) {
		UrlRequest * request = (UrlRequest*)src;
			
		request->OnExecute();
	});
	
	hn_info("url reader start complete");
	return true;
}

void UrlReader::Commit(UrlRequest& request) {
	_queue->Call(request.GetIdx(), &request);
}
