#ifndef __OFS_FILE_DOWNLOADER_H__
#define __OFS_FILE_DOWNLOADER_H__

#include "hnet.h"
#include "rpc/Rpc.h"
#include "api/OfsMaster.pb.h"
#include "api/OfsChunk.pb.h"

namespace ofs {
	class FileDownloader {
	public:
		FileDownloader(api::master::OfsFileService* service, const std::string& token) : _service(service), _token(token) {}

		bool Start(const std::string& remotePath, const std::string& localPath);

	private:
		std::tuple<int32_t, int32_t> GetRemoteFileSize(const std::string& remotePath);
		bool GetBlock(std::ofstream& out, const std::string& remotePath, int32_t index, int32_t size);

	private:
		api::master::OfsFileService* _service;
		std::string _token;
	};
}

#endif //__OFS_FILE_DOWNLOADER_H__
