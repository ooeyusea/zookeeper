#include "Rpc.h"
#include "socket_helper.h"

namespace ofs {
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

		void OfsRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
										google::protobuf::RpcController* controller,
										const google::protobuf::Message* request,
										google::protobuf::Message* response,
										google::protobuf::Closure* done) {
			try {
				std::string data = request->SerializeAsString();

				RpcHeader header;
				header.size = (int32_t)data.size();
				header.method = method->index();
				header.service = olib::BKDRHash(method->service()->full_name().c_str());

				volatile int32_t fd = _fd;

				std::unique_lock<hn_mutex> writeLock(_writeMutex);
				hn_send(fd, (const char*)&header, sizeof(header));
				hn_send(fd, data.data(), (int32_t)data.size());

				std::unique_lock<hn_mutex> readLock(_readMutex);
				writeLock.unlock();

				RpcRet ret;
				olib::SocketReader().ReadType(fd, ret);
				std::string out = olib::SocketReader().ReadBlock(fd, ret.size);

				readLock.unlock();
				
				if (!response->ParseFromString(out)) {
					controller->SetFailed("parse response failed");
				}
			}
			catch (std::exception& e) {
				hn_error("call method {}", e.what());
				controller->SetFailed(e.what());

				if (!_looping) {
					hn_close(_fd);
					_fd = -1;
				}
			}
		}

		bool OfsRpcChannel::Connect(const std::string& ip, int32_t port) {
			_fd = hn_connect(ip.c_str(), port);
			return _fd > 0;
		}

		void OfsRpcChannel::Start(const std::string& ip, int32_t port, const std::function<void()>& fn) {
			_looping = true;
			hn_fork[ip, port, this, fn]{
				while (!_terminate) {
					if (_fd <= 0 || !hn_test_fd(_fd)) {
						hn_close(_fd);

						_fd = hn_connect(ip.c_str(), port);
						fn();
					}

					hn_sleep 100;
				}

				hn_close(_fd);
			};
		}

		bool OfsRpcServer::Start(const std::string& ip, int32_t port) {
			_fd = hn_listen(ip.c_str(), port);
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

				hn_close(_fd);
				--_threadCount;
			};

			return true;
		}
	
		void OfsRpcServer::Stop() {
			hn_close(_fd);

			while (_threadCount > 0) {
				hn_sleep 50;
			}
		}

		void OfsRpcServer::Execute(RpcCommamd& t) {
			OfsRpcController controller(t.fd);
			t.service->CallMethod(t.method, &controller, t.request, t.response, nullptr);

			std::string out = t.response->SerializeAsString();

			RpcRet ret{ (uint32_t)out.size(), false };
			hn_send(t.fd, (const char*)&ret, sizeof(ret));
			hn_send(t.fd, out.data(), (int32_t)out.size());

			delete t.request;
			delete t.response;
		}

		void OfsRpcServer::DealConn(int32_t fd) {
			if (_identifier && !_identifier->Identify(fd))
				return;

			try {
				while (true) {
					RpcHeader header;
					olib::SocketReader().ReadType(fd, header);
					std::string data = olib::SocketReader().ReadBlock(fd, header.size);

					try {
						auto itr = _services.find(header.service);
						if (itr != _services.end()) {
							auto * methodDesc = itr->second->GetDescriptor()->method(header.method);
							if (methodDesc) {
								google::protobuf::Message* request = itr->second->GetRequestPrototype(methodDesc).New();
								google::protobuf::Message* response = itr->second->GetResponsePrototype(methodDesc).New();
								if (request->ParseFromString(data)) {
									if (_is) {
										RpcCommamd cmd{ fd, itr->second, methodDesc, request, response };
										Push(std::forward<RpcCommamd>(cmd));
									}
									else {
										OfsRpcController controller(fd);
										itr->second->CallMethod(methodDesc, &controller, request, response, nullptr);

										std::string out = response->SerializeAsString();

										RpcRet ret{ (uint32_t)out.size(), false };
										hn_send(fd, (const char*)&ret, sizeof(ret));
										hn_send(fd, out.data(), (int32_t)out.size());

										delete request;
										delete response;
									}
									continue;
								}
							}
						}
					}
					catch (std::exception& e) {
						hn_error("handle rpc exception {}", e.what());
					}

					RpcRet ret{ 0, true };
					hn_send(fd, (const char*)&ret, sizeof(ret));
				}
			}
			catch (std::exception& e) {
				hn_error("rpc deal exception {}", e.what());
			}
		}
	}
}
