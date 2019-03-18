#include "service.h"
#include "socket_helper.h"
#define KEEP_ALIVE_TIMEOUT 300

bool ServiceProvider::Start(int32_t port) {
	_fd = hn_listen("0.0.0.0", port);
	if (_fd < 0) {
		hn_error("listen service port on {} failed", port);
		return false;
	}

	++_count;
	hn_fork[this]{
		Process(_fd);
		--_count;
	};

	return true;
}

void ServiceProvider::Stop() {
	_terminate = true;
	hn_shutdown(_fd);

	while (_count > 0) {
		hn_sleep 100;
	}
}

void ServiceProvider::Process(int32_t fd) {
	while (!_terminate) {
		int32_t remoteFd = hn_accept(fd);
		if (remoteFd > 0) {
			++_count;
			hn_fork[remoteFd, this]() {
				try {
					transaction_def::ServiceHeader header;
					while (_terminate) {
						SocketReader(KEEP_ALIVE_TIMEOUT).ReadType(remoteFd, header);
						if (header.op == transaction_def::OP_PING) {
							header.op = transaction_def::OP_PONG;
							hn_send(remoteFd, (const char*)&header, sizeof(header));
						}
						else {
							std::string data = SocketReader(KEEP_ALIVE_TIMEOUT).ReadBlock(remoteFd, header.size - sizeof(header));
							if (header.op == transaction_def::OP_PROPOSE) {
								_executor.Propose(std::move(data), [remoteFd](bool success) {
									char msg[sizeof(transaction_def::ServiceHeader) + sizeof(int8_t)];

									transaction_def::ServiceHeader& header = *(transaction_def::ServiceHeader*)&msg;
									header.op = transaction_def::OP_PROPOSE_ACK;
									header.size = sizeof(msg);
									msg[sizeof(transaction_def::ServiceHeader)] = success ? 1 : 0;

									hn_send(remoteFd, msg, sizeof(msg));
								});
							}
							else if (header.op == transaction_def::OP_READ) {
								std::string result;
								result.resize(sizeof(transaction_def::ServiceHeader), 0);
								_executor.Read(std::move(data), result);

								transaction_def::ServiceHeader& rst = *(transaction_def::ServiceHeader*)result.data();
								rst.op = transaction_def::OP_READ_RESULT;
								rst.size = (int32_t)result.size();
								
								hn_send(remoteFd, result.data(), result.size());
							}
						}
					}
				}
				catch (std::exception& e) {
					hn_error("read from remote failed %s", e.what());
				}

				hn_close(remoteFd);
				--_count;
			};
		}
	}

	hn_close(fd);
}
