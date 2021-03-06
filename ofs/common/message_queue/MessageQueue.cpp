#include "MessageQueue.h"
#include "socket_helper.h"
#include "time_helper.h"
#include "coroutine_waiter.h"

#ifdef WIN32
#define STACK_DYNAMIC_ARR(data, size) char * data = (char *)alloca(size)
#else
#define STACK_DYNAMIC_ARR(data, size) char data[size]
#endif 

#define ID_TIMEOUT 2000
#define LINE_TIMEOUT 30000
#define LINE_CHECK_INTERVAL 5000
#define LINE_HEARBEAT_INTERVAL 15000

namespace ofs {
	namespace mq {
#pragma pack(push, 1)
		enum MessageOpType {
			MOT_MESSAGE,
			MOT_PING,
			MOT_PONG,
			MOT_NODE,
		};

		struct MessageHeader {
			uint32_t size;
			uint32_t msg;
			uint8_t op;
		};
#pragma pack(pop)

		inline std::string toHex(const char* buf, int32_t size) {
			std::string ret;
			for (int32_t i = 0; i < size; ++i) {
				char high = (buf[i] >> 4) & 0x0F;
				char low = buf[i] & 0x0F;

				if (high > 9)
					ret += ('A' + (high - 10));
				else
					ret += ('0' + high);

				if (low > 9)
					ret += ('A' + (low - 10));
				else
					ret += ('0' + low);
			}

			return ret;
		}

		bool MessageQueue::Listen(const std::string& ip, int32_t port) {
			int32_t listenFd = hn_listen(ip.c_str(), port);
			if (listenFd > 0) {
				hn_fork[listenFd, this]{
					while (!_terminate) {
						int32_t fd = hn_accept(listenFd);
						if (fd <= 0)
							break;

						hn_fork[fd, this]{
							try {
								MessageHeader header;
								olib::SocketReader(ID_TIMEOUT).ReadType(fd, header);
								if (header.op != MOT_NODE)
									return;

								hn_info("message queue for {} is opened", (int32_t)header.msg);

								_node[(int32_t)header.msg] = fd;
								Poll((int32_t)header.msg, fd);

								hn_info("message queue for {} is closed", (int32_t)header.msg);
							}
							catch (std::exception&) {
							}

							hn_close(fd);
						};
					}
				};

				return true;
			}
			return false;
		}

		void MessageQueue::Connect(int32_t id, const std::string& ip, int32_t port, const std::function<void()>& fn) {
			{
				std::lock_guard<hn_mutex> guard(_mutex);
				auto itr = _connected.find(ip);
				if (itr != _connected.end()) {
					auto itrPort = std::find(itr->second.begin(), itr->second.end(), port);
					if (itrPort != itr->second.end())
						return;

					itr->second.push_back(port);
				}
				else
					_connected[ip].push_back(port);
			}

			hn_fork[id, ip, port, this, fn]{
				while (!_terminate) {
					int32_t fd = hn_connect(ip.c_str(), port);
					if (fd <= 0)
						continue;

					_node[id] = fd;

					MessageHeader header;
					header.size = 0;
					header.op = MOT_NODE;
					header.msg = (uint32_t)_id;

					hn_send(fd, (const char*)&header, sizeof(header));

					if (fn)
						fn();

					hn_info("message queue for {} is opened", id);
					Poll(id, fd);
					hn_info("message queue for {} is closed", id);
				};
			};
		}

		void MessageQueue::Send(int32_t id, const ::google::protobuf::Message* request) {
			if (_node[id] > 0) {
				int32_t size = request->ByteSize();
				STACK_DYNAMIC_ARR(data, sizeof(MessageHeader) + size);
				if (!request->SerializePartialToArray(data + sizeof(MessageHeader), size))
					return;

				MessageHeader& header = *(MessageHeader*)data;
				header.size = size;
				header.msg = olib::BKDRHash(request->GetDescriptor()->full_name().c_str());
				header.op = MOT_MESSAGE;

				//hn_info("dump {} => {}", header.msg, toHex(data + sizeof(MessageHeader), size));

				hn_send(_node[id], data, sizeof(MessageHeader) + size);
			}
		}

		void MessageQueue::Brocast(std::vector<int32_t> ids, const ::google::protobuf::Message* request) {
			int32_t size = request->ByteSize();
			STACK_DYNAMIC_ARR(data, sizeof(MessageHeader) + size);
			if (!request->SerializePartialToArray(data + sizeof(MessageHeader), size))
				return;

			MessageHeader& header = *(MessageHeader*)data;
			header.size = size;
			header.msg = olib::BKDRHash(request->GetDescriptor()->full_name().c_str());
			header.op = MOT_MESSAGE;

			for (auto id : ids) {
				if (_node[id] > 0)
					hn_send(_node[id], data, sizeof(MessageHeader) + size);
			}
		}

		void MessageQueue::Brocast(const ::google::protobuf::RepeatedField<::google::protobuf::int32 >& ids, const ::google::protobuf::Message* request) {
			int32_t size = request->ByteSize();
			STACK_DYNAMIC_ARR(data, sizeof(MessageHeader) + size);
			if (!request->SerializePartialToArray(data + sizeof(MessageHeader), size))
				return;

			MessageHeader& header = *(MessageHeader*)data;
			header.size = size;
			header.msg = olib::BKDRHash(request->GetDescriptor()->full_name().c_str());
			header.op = MOT_MESSAGE;

			for (auto id : ids) {
				if (_node[id] > 0)
					hn_send(_node[id], data, sizeof(MessageHeader) + size);
			}
		}

		void MessageQueue::Brocast(const ::google::protobuf::Message* request, int32_t except) {
			int32_t size = request->ByteSize();
			STACK_DYNAMIC_ARR(data, sizeof(MessageHeader) + size);
			if (!request->SerializePartialToArray(data + sizeof(MessageHeader), size))
				return;

			MessageHeader& header = *(MessageHeader*)data;
			header.size = size;
			header.msg = olib::BKDRHash(request->GetDescriptor()->full_name().c_str());
			header.op = MOT_MESSAGE;

			for (int32_t i = 0; i < _size; ++i) {
				if (i != except && _node[i] > 0)
					hn_send(_node[i], data, sizeof(MessageHeader) + size);
			}
		}

		void MessageQueue::Poll(int32_t id, int32_t fd) {
			bool stop = false;
			int64_t activeTick = olib::GetTickCount();

			auto co = olib::DoWork([&stop, &activeTick, fd, this]{
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
			});

			try {
				while (!_terminate) {
					MessageHeader header;
					olib::SocketReader(LINE_TIMEOUT).ReadType(fd, header);

					if (header.op == MessageOpType::MOT_PING) {
						header.op = MessageOpType::MOT_PONG;
						hn_send(fd, (const char*)& header, sizeof(header));
					}
					else if (header.op == MessageOpType::MOT_PONG)
						activeTick = olib::GetTickCount();
					else {
						std::string data;
						if (header.size > 0)
							data = olib::SocketReader(LINE_TIMEOUT).ReadBlock(fd, header.size);

						//hn_info("dump {}", toHex(data.c_str(), data.size()));

						auto itr = _funcs.find(header.msg);
						if (itr != _funcs.end()) {
							itr->second(data.c_str(), data.size());

							//hn_trace("do message {} from {}", header.msg, id);
						}
						else {
							hn_warn("unknown message {} from {}", header.msg, id);
						}
					}
				}
			}
			catch (std::exception& e) {
				hn_error("read message from queue {} failed : {}", id, e.what());
			}

			stop = true;
			hn_close(fd);
			co.Wait();
		}
	}
}
