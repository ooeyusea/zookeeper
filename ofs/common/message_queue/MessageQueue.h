#ifndef __COMMON_MESSAGE_H__
#define __COMMON_MESSAGE_H__
#include "hnet.h"
#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/repeated_field.h"
#include "string_id.h"
#include "instruction_sequence/InstructionSequence.h"

namespace ofs {
	namespace mq {
		struct IMessageTokenIdentifier {
			virtual ~IMessageTokenIdentifier() {}

			virtual bool Identify(int32_t fd) = 0;
		};

		class MessageQueue {
		public:
			MessageQueue(int32_t id, int32_t size) : _id(id), _size(size) {
				_node = (int32_t*)malloc(sizeof(int32_t) * size);
				memset(_node, 0, sizeof(int32_t) * size);
			}

			~MessageQueue() {
				delete _node;
			}

			bool Listen(const std::string& ip, int32_t port);
			void Connect(int32_t id, const std::string& ip, int32_t port, const std::function<void ()>& fn = nullptr);

			void Send(int32_t id, const ::google::protobuf::Message* request);
			void Brocast(std::vector<int32_t> ids, const ::google::protobuf::Message* request);
			void Brocast(const ::google::protobuf::RepeatedField<::google::protobuf::int32 >& ids, const ::google::protobuf::Message* request);
			void Brocast(const ::google::protobuf::Message* request, int32_t except = -1);

			inline void SetTokenIndentifier(IMessageTokenIdentifier * identifier) { _identifier = identifier; }

			template <typename T>
			inline void Register(const std::function<void(const T& request)>& fn) {
				uint32_t key = olib::BKDRHash(T().GetDescriptor()->full_name().c_str());
				assert(_funcs.find(key) == _funcs.end());

				_funcs[key] = [fn](const char * data, int32_t size) {
					T request;
					if (request.ParseFromArray(data, size))
						fn(request);
					else {
						hn_error("parse error : {}", request.InitializationErrorString());
					}
				};
			}

		private:
			void Poll(int32_t id, int32_t fd);

		private:
			bool _terminate = false;
			IMessageTokenIdentifier * _identifier = nullptr;
			std::unordered_map<uint32_t, std::function<void(const char * data, int32_t size)>> _funcs;

			int32_t _id;
			int32_t _size;
			int32_t * _node;

			hn_mutex _mutex;
			std::unordered_map<std::string, std::vector<int32_t>> _connected;
		};
	}
}

#endif //__COMMON_RPC_H__
