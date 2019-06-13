#ifndef __COMMON_RPC_H__
#define __COMMON_RPC_H__
#include "hnet.h"
#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "string_id.h"
#include "instruction_sequence/InstructionSequence.h"

namespace ofs {
	namespace rpc {
		class OfsRpcChannel : public google::protobuf::RpcChannel {
		public:
			OfsRpcChannel() {}
			virtual ~OfsRpcChannel() {}

			virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
									google::protobuf::RpcController* controller,
									const google::protobuf::Message* request,
									google::protobuf::Message* response,
									google::protobuf::Closure* done);

			bool Connect(const std::string& ip, int32_t port);
			void Start(const std::string& ip, int32_t port, const std::function<void ()>& fn);
			inline void Stop() {
				_terminate = true;
				hn_sleep 500;
			}

			inline bool IsOpen() const { return _fd > 0; }

		private:
			int32_t _fd = -1;
			bool _terminate = false;
			bool _looping = false;

			hn_mutex _writeMutex;
			hn_mutex _readMutex;
		};

		class OfsRpcController : public google::protobuf::RpcController {
		public:
			OfsRpcController(int32_t fd) : _fd(fd) {}
			virtual ~OfsRpcController() {}


			virtual void Reset() { _failed = false; _error.clear(); }
			virtual bool Failed() const { return _failed; }
			virtual std::string ErrorText() const { return _error; }
			virtual void StartCancel() {}
			virtual void SetFailed(const std::string& reason) {
				_error = reason;
				_failed = true;
			}
			virtual bool IsCanceled() const { return false; }
			virtual void NotifyOnCancel(google::protobuf::Closure* callback) {}

			inline int32_t GetFd() const { return _fd; }

		private:
			int32_t _fd = -1;
			bool _failed = false;
			std::string _error;
		};

		struct IRpcTokenIdentifier {
			virtual ~IRpcTokenIdentifier() {}

			virtual bool Identify(int32_t fd) = 0;
		};

		struct RpcCommamd {
			int32_t fd;
			google::protobuf::Service * service;
			const google::protobuf::MethodDescriptor * method;
			google::protobuf::Message * request;
			google::protobuf::Message * response;
		};

		enum class RpcServerExecuteType {
			RSET_NET,
			RSET_NETCOWORKER,
			RSET_WORKER,
			RSET_INSTRUCTION_SEQUENCE,
		};

		class OfsRpcServer : public instruction_sequence::TExecutor<RpcCommamd> {
		public:
			OfsRpcServer() {}
			~OfsRpcServer() {}

			inline void SetTokenIndentifier(IRpcTokenIdentifier * identifier) { _identifier = identifier; }

			inline void AddService(google::protobuf::Service * service) {
				uint32_t hash = olib::BKDRHash(service->GetDescriptor()->full_name().c_str());
				auto itr = _services.find(hash);
				if (itr != _services.end())
					throw std::logic_error("service name id dup");
				_services[hash] = service;
			}

			bool Start(const std::string& ip, int32_t port);
			void Stop();

		protected:
			virtual void Execute(RpcCommamd& t);

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

#endif //__COMMON_RPC_H__
