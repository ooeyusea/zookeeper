#ifndef __SOCKET_HELPER_H__
#define __SOCKET_HELPER_H__
#include "hnet.h"

namespace yarn {
	namespace socket_helper {
#define TIMEOUT 5000
		struct SocketReader {
			SocketReader(int32_t t) : timeout(t) {}
			SocketReader() : timeout(0) {}

			template <typename T>
			void ReadType(int32_t fd, T& t) {
				int32_t offset = 0;
				while (offset < sizeof(T)) {
					int32_t ret = hn_recv(fd, ((char*)&t) + offset, sizeof(T) - offset, timeout);
					if (ret <= 0)
						throw std::logic_error("read block failed");
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
					if (ret <= 0)
						throw std::logic_error("read block failed");
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
					if (ret <= 0)
						throw std::logic_error("read block failed");
					else
						offset += ret;
				}

				if ((*(T*)&b) != t)
					throw std::logic_error("no expect message");
			}

			std::string ReadBlock(int32_t fd, int32_t size) {
				std::string data;
				data.resize(size, 0);

				int32_t offset = 0;
				while (offset < size) {
					int32_t ret = hn_recv(fd, ((char*)data.data()) + offset, size - offset, timeout);
					if (ret <= 0)
						throw std::logic_error("read block failed");
					else
						offset += ret;
				}
				return data;
			}

			int32_t timeout;
		};
	}
}

#endif //__SOCKET_HELPER_H__
