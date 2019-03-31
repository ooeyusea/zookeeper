#include "URLReader.h"
#include "XmlReader.h"

size_t UrlReader::UrlRequest::WriteData(void *ptr, size_t size, size_t nmemb, void *stream) {
	std::string& data = *(std::string*)stream;
	data.append((const char*)ptr, size * nmemb);

	return size * nmemb;
}

void UrlReader::UrlRequest::OnExecute() {
	curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
	OnExecuteExtern();
	
	_errCode = curl_easy_perform(_curl);
}

void UrlReader::GetRequest::OnExecuteExtern() {

}

void UrlReader::PutRequest::OnExecuteExtern() {

}

void UrlReader::DeleteRequest::OnExecuteExtern() {

}

void UrlReader::PostRequest::OnExecuteExtern() {

}

bool UrlReader::Start(const char * confPath) {
	try {
		olib::XmlReader conf;
		if (!conf.LoadXml(confPath)) {
			return false;
		}

		_threadCount = conf.Root()["mysql"][0].GetAttributeInt32("thread_count");
		hn_info("url reader thread count {}", _threadCount);

		_queue = hn_create_async(_threadCount, true, [this](void * src) {
			UrlRequest * request = (UrlRequest*)src;
			
			request->OnExecute();
		});
	}
	catch (std::exception& e) {
		hn_error("read url reader config failed {}", e.what());
		return false;
	}

	hn_info("mysql start complete");
	return true;
}

void UrlReader::Commit(UrlRequest& request) {
	_queue->Call(request.GetIdx(), &request);
}

UrlReader::GetRequest UrlReader::Get(uint64_t idx, const std::string& url) {

}

UrlReader::PutRequest UrlReader::Put(uint64_t idx, const std::string& url) {

}

UrlReader::DeleteRequest UrlReader::Delete(uint64_t idx, const std::string& url) {

}

UrlReader::PostRequest UrlReader::Post(uint64_t idx, const std::string& url) {

}
