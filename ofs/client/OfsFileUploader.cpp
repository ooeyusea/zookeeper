#include "OfsFileUploader.h"
#include <fstream>

#define BATCH_SIZE (4 * 1024)

namespace ofs {
	bool FileUploader::Start(int32_t oldSize, int32_t blockSize, const std::string& localPath, const std::string& remotePath) {
		std::ifstream local(localPath, std::ios::binary);
		if (local.bad())
			return false;

		int32_t offset = 0;
		char data[BATCH_SIZE];
		while (!local.eof()) {
			if (offset < oldSize) {
				int32_t idx = (offset / blockSize) + 1;

				hn_trace("overwrite block {}[{}:{}]", idx, offset, ((idx * blockSize) > oldSize) ? oldSize : idx * blockSize);

				api::master::WriteRequest request;
				request.set_token(_token);
				request.set_path(remotePath);
				request.set_blockindex(idx);

				api::master::WriteResponse response;
				rpc::OfsRpcController controller(-1);
				_service->Write(&controller, &request, &response, nullptr);

				if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {
					ofs::rpc::OfsRpcChannel channel;
					api::chunk::OfsNodeService* service = new api::chunk::OfsNodeService::Stub(&channel);

					if (!channel.Connect(response.lease().ep().host(), response.lease().ep().port())) {
						return false;
					}

					while (!local.eof()) {
						local.read(data, BATCH_SIZE);
						std::string* buf = new std::string(data, BATCH_SIZE);// local.gcount());

						api::chunk::WriteRequest req;
						req.set_allocated_data(buf);
						req.set_offset(offset);

						auto* lease = req.mutable_lease();
						lease->set_id(response.lease().id());
						lease->set_until(response.lease().until());
						lease->set_version(response.lease().version());
						lease->set_newversion(response.lease().newversion());
						lease->set_key(response.lease().key());
						for (auto server : response.lease().chunkservers())
							lease->add_chunkservers(server);

						api::chunk::WriteResponse rsp;
						controller.Reset();
						service->Write(&controller, &req, &rsp, nullptr);

						if (controller.Failed() || rsp.errcode() != api::chunk::ErrorCode::EC_NONE) {
							if (rsp.errcode() != api::chunk::ErrorCode::EC_LEASE_EXPIRE)
								return false;

							local.seekg(-local.gcount(), std::ios::cur);
							break;
						}

						offset += local.gcount();

						if (offset % blockSize == 0)
							break;

						if (offset >= oldSize)
							break;
					}
				}
				else
					break;
			}
			else {
				api::master::AppendRequest request;
				request.set_token(_token);
				request.set_path(remotePath);

				api::master::AppendResponse response;
				rpc::OfsRpcController controller(-1);
				_service->Append(&controller, &request, &response, nullptr);

				if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {
					ofs::rpc::OfsRpcChannel channel;
					api::chunk::OfsNodeService* service = new api::chunk::OfsNodeService::Stub(&channel);

					if (!channel.Connect(response.lease().ep().host(), response.lease().ep().port())) {
						return false;
					}

					while (!local.eof()) {
						local.read(data, BATCH_SIZE);
						std::string* buf = new std::string(data, BATCH_SIZE);// local.gcount());

						api::chunk::AppendRequest req;
						req.set_allocated_data(buf);

						auto* lease = req.mutable_lease();
						lease->set_id(response.lease().id());
						lease->set_until(response.lease().until());
						lease->set_version(response.lease().version());
						lease->set_newversion(response.lease().newversion());
						lease->set_key(response.lease().key());
						for (auto server : response.lease().chunkservers())
							lease->add_chunkservers(server);

						api::chunk::AppendResponse rsp;
						controller.Reset();
						service->Append(&controller, &req, &rsp, nullptr);

						if (controller.Failed() || rsp.errcode() != api::chunk::ErrorCode::EC_NONE) {
							if (rsp.errcode() != api::chunk::ErrorCode::EC_BLOCK_FULL && rsp.errcode() != api::chunk::ErrorCode::EC_LEASE_EXPIRE)
								return false;

							local.seekg(-local.gcount(), std::ios::cur);
							break;
						}
					}
				}
				else
					break;
			}
		}

		return false;
	}

}
