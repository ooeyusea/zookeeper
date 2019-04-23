#include "OfsClient.h"
#include <iostream>

namespace ofs {
	std::vector<std::string> Split(const std::string& line, const char * sep) {
		std::vector<std::string> ret;
		
		std::string::size_type start = 0;
		std::string::size_type pos = line.find_first_of(sep, start);
		while (pos != std::string::npos) {
			if (pos != start)
				ret.push_back(line.substr(start, pos - start));

			start = pos + 1;
			pos = line.find_first_of(sep, start);
		}

		if (start < line.size())
			ret.push_back(line.substr(start));

		return ret;
	}

	Client::Client() {

	}

	bool Client::Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password) {
		_service = new api::OfsFileService::Stub(&_channel);

		_username = username;
		_host = host;

		if (!_channel.Connect(host, port)) {
			std::cerr << "connect to ofs failed" << std::endl;
			return false;
		}

		if (!Login(username, password)) {
			std::cerr << "login ofs failed" << std::endl;
			return false;
		}

		return true;
	}

	void Client::DoCommand(const std::string& line) {
		std::vector<std::string> units = Split(line, " \t");
		std::vector<char*> ptrs;
		for (auto& unit : units)
			ptrs.push_back((char*)unit.c_str());

		int32_t argc = (int32_t)ptrs.size();
		char ** argv = ptrs.data();

		auto itr = _commands.find(argv[0]);
		if (itr != _commands.end()) {
			(this->*(itr->second))(argc, argv);
		}
		else {
			std::cout << argv[0] << " : command not found" << std::endl;
		}
	}

	void Client::Slash() {
		std::cout << "[" << _username << "@" << _host << " " << _currentPath << "]";
		if (_supper)
			std::cout << "# " << std::endl;
		else
			std::cout << "$ " << std::endl;
	}

	bool Client::Login(const std::string& username, const std::string& password) {
		api::LoginReq request;
		request.set_name(username);
		request.set_password(password);

		api::LoginResponse response;
		rpc::OfsRpcController controller;
		_service->Login(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::ErrorCode::EC_NONE) {
			_token = response.token();
			return true;
		}

		return false;
	}
}
