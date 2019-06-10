#ifndef __OFS_FILEUPLOADER_H__
#define __OFS_FILEUPLOADER_H__
#include "hnet.h"
#include "rpc/Rpc.h"
#include "api/OfsMaster.pb.h"
#include "api/OfsChunk.pb.h"

namespace ofs {
	class FileUploader {
	public:
		FileUploader(const std::string& local) : _local(local) {}
		~FileUploader() {}

		void Upload(api::master::OfsFileService * service, const std::string& path, const std::string& token);

	private:
		bool AskAppend(api::master::OfsFileService* service, const std::string& path, const std::string& token);

	private:
		std::string _local;
	};
}

#endif //__OFS_FILEUPLOADER_H__
