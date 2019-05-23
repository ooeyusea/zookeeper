#include "MessageQueue.h"
#include "socket_helper.h"
#include "time_helper.h"

#ifdef WIN32
#define STACK_DYNAMIC_ARR(data, size) char * data = (char *)alloca(size)
#else
#define STACK_DYNAMIC_ARR(data, size) char data[size]
#endif 

#define LINE_TIMEOUT 30000
#define LINE_CHECK_INTERVAL 5000
#define LINE_HEARBEAT_INTERVAL 15000

namespace ofs {
	namespace mq {
#pragma pack(push, 1)
		enum MessageOpType {
			MOT_MESSAGE,
			MOT_PING,
			MOT_PONG
		};

		struct MessageHeader {
			uint32_t size;
			uint32_t msg;
			uint8_t op;
		};
#pragma pack(pop)

		bool MessageQueue::Listen(const std::string& ip, int32_t port) {
			int32_t listenFd = hn_listen(ip.c_str(), port);
			if (listenFd > 0) {
				hn_fork[listenFd, this]{
					while (!_terminate) {
						int32_t fd = hn_accept(listenFd);
						if (fd <= 0)
							break;

						hn_fork[fd, this]{
							int32_t id = _identifier->Identify(fd);
							_node[id] = fd;

							Poll(id, fd);
						};
					}
				};

				return true;
			}
			return false;
		}

		void MessageQueue::Connect(const std::string& ip, int32_t port) {
			hn_fork[ip, port, this]{
				while (!_terminate) {
					int32_t fd = hn_connect(ip.c_str(), port);
					if (fd <= 0)
						continue;

					int32_t id = _identifier->Identify(fd);
					_node[id] = fd;

					Poll(id, fd);
				};
			};
		}

		void MessageQueue::Send(int32_t id, const ::google::protobuf::Message* request) {
			if (_node[id] > 0) {
				int32_t size = request->ByteSize();
#ifdef WIN32
				STACK_DYNAMIC_ARR(data, sizeof(MessageHeader) + size);
#else
				char data[sizeof(MessageHeader) + size];
#endif 
				if (!request->SerializePartialToArray(data + sizeof(MessageHeader), size))
					return;

				MessageHeader& header = *(MessageHeader*)data;
				header.size = size;
				header.msg = olib::BKDRHash(request->GetDescriptor()->full_name().c_str());
				header.op = MOT_MESSAGE;

				hn_send(_node[id], data, size);
			}
		}

		void MessageQueue::Poll(int32_t id, int32_t fd) {
			bool stop = false;
			int64_t activeTick = olib::GetTickCount();

			hn_fork[&stop, &activeTick, fd, this]{
				int64_t sendTick = olib::GetTickCount();
				while (!stop && !_terminate) {
					hn_sleep LINE_CHECK_INTERVAL;

					int64_t now = olib::GetTickCount();
					if (now - activeTick > LINE_TIMEOUT) {
						hn_shutdown(fd);
						break;
					}

					if (now - sendTick >= LINE_HEARBEAT_INTERVAL) {
						MessageHeader header;
						header.size = 0;
						header.op = MessageOpType::MOT_PING;

						hn_send(fd, (const char*)&header, sizeof(header));
					}
				}
			};

			while (!_terminate) {
				MessageHeader header;
				olib::SocketReader(LINE_TIMEOUT).ReadType(fd, header);

				if (header.op == MessageOpType::MOT_PING) {
					header.op = MessageOpType::MOT_PONG;
					hn_send(fd, (const char*)&header, sizeof(header));
				}
				else if (header.op == MessageOpType::MOT_PONG)
					activeTick = olib::GetTickCount();
				else {
					std::string data;
					if (header.size > 0)
						data = olib::SocketReader(LINE_TIMEOUT).ReadBlock(fd, header.size);
					
					auto itr = _funcs.find(header.msg);
					if (itr != _funcs.end())
						itr->second(data.c_str(), data.size());
				}
			}

			stop = true;
			hn_close(fd);
		}
	}
}
