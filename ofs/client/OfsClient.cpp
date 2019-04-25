#include "OfsClient.h"
#include <iostream>
#include "args.h"
#include <stack>

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
		_commands["cd"] = &Client::ChangeDir;
		_commands["mkdir"] = &Client::Mkdir;
		_commands["touch"] = &Client::Touch;
		_commands["rm"] = &Client::Remove;
		_commands["ls"] = &Client::List;
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

		if (!ReadRootPath()) {
			std::cerr << "read root path failed" << std::endl;
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

	bool Client::ReadRootPath() {
		api::FileStatusRequest request;
		request.set_token(_token);
		request.set_path("/");

		api::FileStatusRespone response;
		rpc::OfsRpcController controller;
		_service->Status(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::ErrorCode::EC_NONE) {
			_root.name = response.file().name();
			_root.owner = response.file().owner();
			_root.ownerGroup = response.file().group();
			_root.authority = response.file().authority();
			_root.dir = response.file().dir();
			_root.createTime = response.file().createtime();
			_root.updateTime = response.file().updatetime();
			_root.size = response.file().size();

			return true;
		}

		return false;
	}

	void Client::ChangeDir(int32_t argc, char ** argv) {
		args::ArgumentParser parser("This is a ofs cd program.", "");
		args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
		args::Positional<std::string> path(parser, "path", "path to change");

		try
		{
			parser.ParseCLI(argc, argv);
		}
		catch (args::Help) {
			std::cout << parser;
			return;
		}
		catch (args::ParseError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}
		catch (args::ValidationError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}

		std::string realPath = FindReal(args::get(path));
		if (realPath.empty())
			return;

		if (!Expand(realPath.c_str()))
			return;

		_currentPath = std::move(realPath);
	}

	void Client::Mkdir(int32_t argc, char ** argv) {
		args::ArgumentParser parser("This is a ofs mkdir program.", "");
		args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
		args::Positional<std::string> path(parser, "path", "path");

		try
		{
			parser.ParseCLI(argc, argv);
		}
		catch (args::Help) {
			std::cout << parser;
			return;
		}
		catch (args::ParseError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}
		catch (args::ValidationError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}

		std::string realPath;
		std::string realName;
		std::tie(realPath, realName) = FindRealWithName(args::get(path));

		api::MakeDirRequest request;
		request.set_token(_token);
		request.set_directory(realPath);
		request.set_name(realName);
		request.set_authority(api::AuthorityType::AT_OWNER_READ | api::AuthorityType::AT_OWNER_WRITE | api::AuthorityType::AT_GROUP_READ | api::AuthorityType::AT_OTHER_READ);

		api::MakeDirResponse response;
		rpc::OfsRpcController controller;

		_service->MakeDir(&controller, &request, &response, nullptr);

		if (!controller.Failed()) {
			if (response.errcode() == api::ErrorCode::EC_NONE) {

			}
			else {

			}
		}
		else {

		}
	}

	void Client::Touch(int32_t argc, char ** argv) {
		args::ArgumentParser parser("This is a ofs touch program.", "");
		args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
		args::Positional<std::string> path(parser, "path", "path");

		try
		{
			parser.ParseCLI(argc, argv);
		}
		catch (args::Help) {
			std::cout << parser;
			return;
		}
		catch (args::ParseError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}
		catch (args::ValidationError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}

		std::string realPath;
		std::string realName;
		std::tie(realPath, realName) = FindRealWithName(args::get(path));

		api::CreateFileRequest request;
		request.set_token(_token);
		request.set_directory(realPath);
		request.set_name(realName);
		request.set_authority(api::AuthorityType::AT_OWNER_READ | api::AuthorityType::AT_OWNER_WRITE | api::AuthorityType::AT_GROUP_READ | api::AuthorityType::AT_OTHER_READ);

		api::CreateFileResponse response;
		rpc::OfsRpcController controller;

		_service->Create(&controller, &request, &response, nullptr);

		if (!controller.Failed()) {
			if (response.errcode() == api::ErrorCode::EC_NONE) {

			}
			else {

			}
		}
		else {

		}
	}

	void Client::Remove(int32_t argc, char ** argv) {
		args::ArgumentParser parser("This is a ofs rm program.", "");
		args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
		args::Positional<std::string> path(parser, "path", "path");

		try
		{
			parser.ParseCLI(argc, argv);
		}
		catch (args::Help) {
			std::cout << parser;
			return;
		}
		catch (args::ParseError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}
		catch (args::ValidationError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}

		std::string realPath = FindReal(args::get(path));

		api::RemoveRequest request;
		request.set_token(_token);
		request.set_path(realPath);

		api::RemoveResponse response;
		rpc::OfsRpcController controller;

		_service->Remove(&controller, &request, &response, nullptr);

		if (!controller.Failed()) {
			if (response.errcode() == api::ErrorCode::EC_NONE) {

			}
			else {

			}
		}
		else {

		}
	}

	void Client::List(int32_t argc, char ** argv) {
		args::ArgumentParser parser("This is a ofs ls program.", "");
		args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });

		try
		{
			parser.ParseCLI(argc, argv);
		}
		catch (args::Help) {
			std::cout << parser;
			return;
		}
		catch (args::ParseError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}
		catch (args::ValidationError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return;
		}

		if (Expand(_currentPath.c_str())) {

		}
	}

	int32_t Client::Expand(const char * path) {
		Node * node = GetNode(_root, path);
		if (node && !node->dir) {
			return false;
		}

		if (!node || !node->expand) {
			api::ListRequest request;
			request.set_token(_token);
			request.set_path(path);

			api::ListResponse response;
			rpc::OfsRpcController controller;

			_service->List(&controller, &request, &response, nullptr);

			if (!controller.Failed()) {
				if (response.errcode() == api::ErrorCode::EC_NONE) {
					if (!node)
						node = CreateNode(_root, path);
					
					for (int32_t i = 0; i < (int32_t)response.files_size(); ++i) {
						const auto& file = response.files(i);

						Node * child = new Node;
						child->name = file.name();
						child->owner = file.owner();
						child->ownerGroup = file.group();
						child->authority = file.authority();
						child->dir = file.dir();
						child->createTime = file.createtime();
						child->updateTime = file.updatetime();
						child->size = file.size();
						child->expand = false;

						node->children.emplace_back(node);
					}
				}
				else {
					return false;
				}
			}
			else {
				return false;
			}
		}

		return true;
	}

	std::string Client::FindReal(const std::string& path) {
		std::vector<std::string> units = Split(path, "/");
		std::vector<std::string> stacks;
		for (auto& str : units) {
			if (str == "..") {
				if (stacks.empty())
					return "";

				stacks.pop_back();
			}
			else if (str != ".")
				stacks.emplace_back(std::move(str));	
		}

		std::string ret;
		for (auto& str : stacks) {
			ret += "/";
			ret += str;
		}

		if (ret.empty())
			ret = "/";

		return ret;
	}

	std::tuple<std::string, std::string> Client::FindRealWithName(const std::string& path) {
		std::string real = FindReal(path);
		std::string name;

		if (!real.empty()) {
			auto pos = real.find_last_of('/');
			if (pos != std::string::npos) {
				name = real.substr(pos + 1);
				real.erase(pos);
			}
		}
		return std::make_tuple(std::move(real), std::move(name));
	}

	Client::Node * Client::GetNode(Node& node, const char * path) {
		if (*path == '/')
			++path;

		std::string subDir;
		const char * next = nullptr;
		std::tie(subDir, next) = FetchSubPath(path);

		if (subDir.empty())
			return &node;

		auto itr = std::find_if(node.children.begin(), node.children.end(), [&subDir](Node * node) {
			return node->name == subDir;
		});

		if (itr == node.children.end())
			return nullptr;

		if (next && !(*itr)->dir)
			return nullptr;

		return !next ? (*itr) : GetNode(*(*itr), next);
	}

	Client::Node * Client::CreateNode(Node& node, const char * path) {
		if (*path == '/')
			++path;

		std::string subDir;
		const char * next = nullptr;
		std::tie(subDir, next) = FetchSubPath(path);

		if (subDir.empty())
			return &node;

		auto itr = std::find_if(node.children.begin(), node.children.end(), [&subDir](Node * node) {
			return node->name == subDir;
		});

		Node * child = nullptr;
		if (itr == node.children.end()) {
			child = new Node;
			child->name = subDir;
			child->dir = true;
			child->expand = false;
			node.children.emplace_back(child);
		}
		else
			child = *itr;

		if (next)
			return CreateNode(*child, next);

		return child;
	}
}
