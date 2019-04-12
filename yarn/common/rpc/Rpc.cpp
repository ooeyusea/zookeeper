#include "Rpc.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "socket_helper.h"

namespace yarn {
	namespace rpc {
#pragma pack(push, 1)
		struct RpcHeader {
			uint32_t size;
			uint32_t service;
			uint16_t method;
		};

		struct RpcRet {
			uint32_t size;
			bool failed;
		};
#pragma pack(pop)

		void YarnRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
										google::protobuf::RpcController* controller,
										const google::protobuf::Message* request,
										google::protobuf::Message* response,
										google::protobuf::Closure* done) {

			
		}


		bool YarnRpcServer::Start(const char * ip, int32_t port) {
			_fd = hn_listen(ip, port);
			if (_fd <= 0)
				return false;

			++_threadCount;
			hn_fork[this]{
				while (true) {
					int32_t remoteFd = hn_accept(_fd);
					if (remoteFd < 0)
						break;

					++_threadCount;
					hn_fork[remoteFd, this]{
						DealConn(remoteFd);
						
						hn_close(remoteFd);
						--_threadCount;
					};
				}

				--_threadCount;
			};
		}
	
		void YarnRpcServer::Stop() {
			hn_close(_fd);

			while (_threadCount > 0) {
				hn_sleep 50;
			}
		}

		void YarnRpcServer::DealConn(int32_t fd) {
			if (_identifier && !_identifier->Identify(fd))
				return;

			try {
				while (true) {
					RpcHeader header;
					socket_helper::SocketReader().ReadType(fd, header);
					std::string data = socket_helper::SocketReader().ReadBlock(fd, header.size);

					try {
						auto itr = _services.find(header.service);
						if (itr != _services.end()) {
							auto * methodDesc = itr->second->GetDescriptor()->method(header.method);
							if (methodDesc) {
								google::protobuf::Message* request = itr->second->GetRequestPrototype(methodDesc).New();
								google::protobuf::Message* response = itr->second->GetResponsePrototype(methodDesc).New();
								if (request->ParseFromString(data)) {
									YarnRpcController controller;
									itr->second->CallMethod(methodDesc, &controller, request, response, nullptr);
									
									std::string out = response->SerializeAsString();

									RpcRet ret{ (uint32_t)out.size(), false };
									hn_send(fd, (const char*)&ret, sizeof(ret));
									hn_send(fd, out.data(), out.size());

									delete request;
									continue;
								}
							}
						}
					}
					catch (std::exception& e) {
						hn_error("handle rpc exception %s", e.what());
					}

					RpcRet ret{ 0, true };
					hn_send(fd, (const char*)&ret, sizeof(ret));
				}
			}
			catch (std::exception& e) {
				hn_error("rpc deal exception %s", e.what());
			}
		}
	}
}
