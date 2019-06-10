#include "OfsFileDownloader.h"
#include <fstream>

namespace ofs {
	bool FileDownloader::Start(const std::string& remotePath, const std::string& localPath) {
		int32_t size = -1;
		int32_t blockSize = 0;
		std::tie(size, blockSize) = GetRemoteFileSize(remotePath);
		if (size < 0 || blockSize == 0)
			return false;

		std::ofstream local(localPath);
		if (size > 0) {
			int32_t blockCount = size / blockSize + (size % blockSize == 0 ? 0 : 1);
			for (int32_t i = 1; i <= blockCount; ++i) {
				if (!GetBlock(local, remotePath, i, (i != blockCount ? blockSize : (size - ((i - 1) * blockSize)))))
					return false;
			}
		}

		local.flush();
		local.close();
		return true;
	}

	std::tuple<int32_t, int32_t> FileDownloader::GetRemoteFileSize(const std::string& remotePath) {
		api::master::FileStatusRequest request;
		request.set_token(_token);
		request.set_path(remotePath);

		api::master::FileStatusRespone response;
		rpc::OfsRpcController controller(-1);
		_service->Status(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {
			if (!response.file().dir())
				return std::make_tuple(response.file().size(), response.file().blocksize());
		}
		return std::make_tuple(-1, 0);
	}

	bool FileDownloader::GetBlock(std::ofstream& out, const std::string& remotePath, int32_t index, int32_t size) {
		
		api::master::ReadRequest request;
		request.set_token(_token);
		request.set_path(remotePath);
		request.set_blockindex(index);

		api::master::ReadResponse response;
		rpc::OfsRpcController controller(-1);
		_service->Read(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {
			int64_t blockId = response.id();
			
			int32_t offset = 0;
			for (int32_t i = 0; i < response.eps_size(); ++i) {
				if (offset >= size)
					return true;

				ofs::rpc::OfsRpcChannel channel;
				api::chunk::OfsNodeService * service = new api::chunk::OfsNodeService::Stub(&channel);

				if (!channel.Connect(response.eps(i).host(), response.eps(i).port())) {
					continue;
				}

				while (offset < size) {
					api::chunk::ReadRequest req;
					req.set_blockid(blockId);
					req.set_offset(offset);

					api::chunk::ReadResponse rsp;
					controller.Reset();
					service->Read(&controller, &req, &rsp, nullptr);

					if (!controller.Failed() && response.errcode() == api::chunk::ErrorCode::EC_NONE) {
						const std::string& data = rsp.data();
						out.write(data.c_str(), data.size());

						if (out.bad())
							return false;

						offset += data.size();
					}
					else
						break;
				}
			}
		}
		return false;
	}
}
