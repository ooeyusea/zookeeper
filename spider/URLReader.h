#ifndef __URLREADER_H__
#define __URLREADER_H__
#include "hnet.h"
#include "curl/curl.h"

class UrlReader {
	enum class UrlType {
		URL_TYPE_GET,
		URL_TYPE_POST,
		URL_TYPE_DELETE,
		URL_TYPE_PUT,
	};
public:
	class UrlRequest {
		friend class UrlReader;
	public:
		~UrlRequest() {
			if (_headers)
				curl_slist_free_all(_headers);

			if (_curl)
				curl_easy_cleanup(_curl);
		}

		inline uint64_t GetIdx() const { return _idx; }
		inline int32_t GetResult() const { return _errCode; }

		inline UrlRequest& SetJson(bool json) {
			if (_curl)
				_headers = curl_slist_append(_headers, "Content-Type:application/json;charset=UTF-8");
			return *this;
		}

		inline UrlRequest& SetTimeout(int32_t timeout) {
			if (_curl)
				curl_easy_setopt(_curl, CURLOPT_TIMEOUT, timeout);
			return *this;
		}

		inline UrlRequest& SetField(const char * fields) {
			if (_curl)
				curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, fields);
			return *this;
		}

		inline UrlRequest& AddHeader(const char * header) {
			if (_curl)
				_headers = curl_slist_append(_headers, header);
			return *this;
		}

		inline UrlRequest& StartCookie() {
			if (_curl)
				curl_easy_setopt(_curl, CURLOPT_COOKIEFILE, "");
			return *this;
		}

		inline UrlRequest& SetCookie(const char * cookie) {
			if (_curl)
				curl_easy_setopt(_curl, CURLOPT_COOKIE, cookie);
			return *this;
		}

		inline void Commit() {
			if (!_curl) {
				_errCode = CURLE_COULDNT_CONNECT;
				return;
			}

			UrlReader::Instance().Commit(*this);
		}

		inline void IteratorCookie(const std::function<void(const char *)>& f) {
			if (_curl) {
				struct curl_slist *cookies = NULL;
				curl_easy_getinfo(_curl, CURLINFO_COOKIELIST, &cookies);		//获得cookie数据

				while (cookies) {
					f(cookies->data);
					cookies = cookies->next;
				}
			}
		}

		static size_t WriteData(void *ptr, size_t size, size_t nmemb, void *stream);

	private:
		UrlRequest(uint64_t idx, const char* url, UrlType type) : _idx(_idx) {
			_curl = curl_easy_init();
			if (_curl) {
				curl_easy_setopt(_curl, CURLOPT_URL, url);
				curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, WriteData);
				curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_content);
				curl_easy_setopt(_curl, CURLOPT_TIMEOUT, 10);
				curl_easy_setopt(_curl, CURLOPT_VERBOSE, 0L);
				curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);

				switch (type) {
				case UrlType::URL_TYPE_GET: break;
				case UrlType::URL_TYPE_PUT: curl_easy_setopt(_curl, CURLOPT_PUT, 1); break;
				case UrlType::URL_TYPE_DELETE: curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "DELETE"); break;
				case UrlType::URL_TYPE_POST: curl_easy_setopt(_curl, CURLOPT_POST, 1); break;
				}
			}
		}

		void OnExecute();

	private:
		uint64_t _idx;

		CURLcode _errCode;
		std::string _content;

		CURL * _curl = nullptr;
		curl_slist* _headers = nullptr;
	};

	friend class UrlRequest;

public:
	static UrlReader& Instance() {
		static UrlReader g_instance;
		return g_instance;
	}

	bool Start(const char * confPath);

	inline UrlRequest Get(uint64_t idx, const char * url) { return UrlRequest(idx, url, UrlType::URL_TYPE_GET); }
	inline UrlRequest Put(uint64_t idx, const char * url) { return UrlRequest(idx, url, UrlType::URL_TYPE_PUT); }
	inline UrlRequest Delete(uint64_t idx, const char * url) { return UrlRequest(idx, url, UrlType::URL_TYPE_DELETE); }
	inline UrlRequest Post(uint64_t idx, const char * url) { return UrlRequest(idx, url, UrlType::URL_TYPE_POST); }

protected:
	void Commit(UrlRequest& request);

private:
	UrlReader() {}
	~UrlReader() {}

	hyper_net::IAsyncQueue * _queue = nullptr;
	int32_t _threadCount;
};

#endif //__URLREADER_H__