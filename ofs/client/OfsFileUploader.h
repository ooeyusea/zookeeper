#ifndef __OFS_FILEUPLOADER_H__
#define __OFS_FILEUPLOADER_H__
#include "hnet.h"
#include "rpc/Rpc.h"
#include "api/OfsMaster.pb.h"
#include "api/OfsChunk.pb.h"

namespace ofs {
	class FileUploader {
	public:
		FileUploader(api::master::OfsFileService* service, const std::string& token) : _service(service), _token(token) {}
		~FileUploader() {}

		bool Start(const std::string& localPath, const std::string& remotePath);

	private:
		api::master::OfsFileService* _service;
		std::string _token;
	};
}

#endif //__OFS_FILEUPLOADER_H__
