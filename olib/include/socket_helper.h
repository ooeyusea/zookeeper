#ifndef __SOCKET_HELPER_H__
#define __SOCKET_HELPER_H__
#include "hnet.h"

namespace olib {
	class NetErrorException : std::exception {
	public:
		NetErrorException(const std::string& msg) : std::exception(msg.c_str()) {}
		NetErrorException(const char * msg) : std::exception(msg) {}
	};

	class NetTimeoutException : std::exception {
	public:
		NetTimeoutException(const std::string& msg) : std::exception(msg.c_str()) {}
		NetTimeoutException(const char* msg) : std::exception(msg) {}
	};

	class NetNotException : std::exception {
	public:
		NetNotException(const std::string& msg) : std::exception(msg.c_str()) {}
		NetNotException(const char* msg) : std::exception(msg) {}
	};

	struct SocketReader {
		SocketReader(int32_t t) : timeout(t) {}
		SocketReader() : timeout(0) {}

		template <typename T>
		void ReadType(int32_t fd, T& t) {
			int32_t offset = 0;
			while (offset < sizeof(T)) {
				int32_t ret = hn_recv(fd, ((char*)&t) + offset, sizeof(T) - offset, timeout);
				if (ret <= 0) {
					if (ret == -2)
						throw NetTimeoutException("read block time out");
					else
						throw NetErrorException("read block failed");
				}
				else
					offset += ret;
			}
		}

		template <typename T, typename B>
		void ReadRest(int32_t fd, B& b) {
			static_assert(sizeof(T) <= sizeof(B), "T must small than B");

			int32_t offset = sizeof(T);
			while (offset < sizeof(B)) {
				int32_t ret = hn_recv(fd, ((char*)&b) + offset, sizeof(B) - offset, timeout);
				if (ret <= 0) {
					if (ret == -2)
						throw NetTimeoutException("read block time out");
					else
						throw NetErrorException("read block failed");
				}
				else
					offset += ret;
			}
		}

		template <typename T, typename B>
		void ReadMessage(int32_t fd, T t, B& b) {
			static_assert(sizeof(T) <= sizeof(B), "T must small than B");

			int32_t offset = 0;
			while (offset < sizeof(T)) {
				int32_t ret = hn_recv(fd, ((char*)&b) + offset, sizeof(T) - offset, timeout);
				if (ret <= 0) {
					if (ret == -2)
						throw NetTimeoutException("read block time out");
					else
						throw NetErrorException("read block failed");
				}
				else
					offset += ret;
			}

			if ((*(T*)&b) != t)
				throw NetNotException("no expect message");
		}

		std::string ReadBlock(int32_t fd, int32_t size) {
			std::string data;
			data.resize(size, 0);

			int32_t offset = 0;
			while (offset < size) {
				int32_t ret = hn_recv(fd, ((char*)data.data()) + offset, size - offset, timeout);
				if (ret <= 0) {
					if (ret == -2)
						throw NetTimeoutException("read block time out");
					else
						throw NetErrorException("read block failed");
				}
				else
					offset += ret;
			}
			return data;
		}

		int32_t timeout;
	};
}

#endif //__SOCKET_HELPER_H__
