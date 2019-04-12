#ifndef __YARN_RPC_H__
#define __YARN_RPC_H__
#include "hnet.h"
#include "google/protobuf/service.h"
#include "string_id.h"

namespace yarn {
	namespace rpc {
		class YarnRpcChannel : public google::protobuf::RpcChannel {
		public:
			YarnRpcChannel() {}
			virtual ~YarnRpcChannel() {}

			virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
									google::protobuf::RpcController* controller,
									const google::protobuf::Message* request,
									google::protobuf::Message* response,
									google::protobuf::Closure* done);

			void Start(const char * ip, int32_t port);

		private:
			int32_t _fd = -1;
		};

		class YarnRpcController : public google::protobuf::RpcController {
		public:
			YarnRpcController() {}
			virtual ~YarnRpcController() {}


			virtual void Reset() {}
			virtual bool Failed() const { return _failed; }
			virtual std::string ErrorText() const { return _error; }
			virtual void StartCancel() {}
			virtual void SetFailed(const std::string& reason) {
				_error = reason;
				_failed = true;
			}
			virtual bool IsCanceled() const { return false; }
			virtual void NotifyOnCancel(google::protobuf::Closure* callback) {}

		private:
			bool _failed = false;
			std::string _error;
		};

		struct IRpcTokenIdentifier {
			virtual ~IRpcTokenIdentifier() {}

			virtual bool Identify(int32_t fd) = 0;
		};

		class YarnRpcServer {
		public:
			YarnRpcServer() {}
			~YarnRpcServer() {}

			inline void SetTokenIndentifier(IRpcTokenIdentifier * identifier) { _identifier = identifier; }

			inline void AddService(google::protobuf::Service * service) {
				uint32_t hash = string_helper::BKDRHash(service->GetDescriptor()->full_name().c_str());
				auto itr = _services.find(hash);
				if (itr != _services.end())
					throw std::logic_error("service name id dup");
				_services[hash] = service;
			}

			bool Start(const char * ip, int32_t port);
			void Stop();

		private:
			void DealConn(int32_t fd);

		private:
			std::unordered_map<uint32_t, google::protobuf::Service *> _services;
			IRpcTokenIdentifier * _identifier = nullptr;

			std::atomic<int32_t> _threadCount = 0;
			int32_t _fd = -1;
		};
	}
}

#endif //__YARN_RPC_H__
