#include "OfsFileUploader.h"

namespace ofs {
	void FileUploader::Upload(api::master::OfsFileService* service, const std::string& path, const std::string& token) {

	}


	bool FileUploader::AskAppend(api::master::OfsFileService* service, const std::string& path, const std::string& token) {
		api::master::AppendRequest request;
		request.set_token(token);
		request.set_path(path);

		api::master::AppendResponse response;
		rpc::OfsRpcController controller(-1);
		service->Append(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {

			return true;
		}

		return false;
	}
}