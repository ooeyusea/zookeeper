#include "OfsClient.h"
#include <iostream>
#include "args.h"
#include <stack>
#include "time_helper.h"
#include "OfsFileDownloader.h"

#define EXPAND_EXPIRE_INTERVAL MINUTE

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
		_commands["get"] = &Client::Get;
	}

	bool Client::Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password) {
		_service = new api::master::OfsFileService::Stub(&_channel);

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
			std::cout << "# ";
		else
			std::cout << "$ ";
	}

	bool Client::Login(const std::string& username, const std::string& password) {
		api::master::LoginReq request;
		request.set_name(username);
		request.set_password(password);

		api::master::LoginResponse response;
		rpc::OfsRpcController controller(-1);
		_service->Login(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {
			_token = response.token();
			return true;
		}

		return false;
	}

	bool Client::ReadRootPath() {
		api::master::FileStatusRequest request;
		request.set_token(_token);
		request.set_path("/");

		api::master::FileStatusRespone response;
		rpc::OfsRpcController controller(-1);
		_service->Status(&controller, &request, &response, nullptr);

		if (!controller.Failed() && response.errcode() == api::master::ErrorCode::EC_NONE) {
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

		api::master::MakeDirRequest request;
		request.set_token(_token);
		request.set_directory(realPath);
		request.set_name(realName);
		request.set_authority(api::master::AuthorityType::AT_OWNER_READ | api::master::AuthorityType::AT_OWNER_WRITE | api::master::AuthorityType::AT_GROUP_READ | api::master::AuthorityType::AT_OTHER_READ);

		api::master::MakeDirResponse response;
		rpc::OfsRpcController controller(-1);

		_service->MakeDir(&controller, &request, &response, nullptr);

		if (!controller.Failed()) {
			if (response.errcode() == api::master::ErrorCode::EC_NONE) {
				Node* node = GetNode(_root, realPath.c_str());
				if (node)
					node->expand = false;
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

		api::master::CreateFileRequest request;
		request.set_token(_token);
		request.set_directory(realPath);
		request.set_name(realName);
		request.set_authority(api::master::AuthorityType::AT_OWNER_READ | api::master::AuthorityType::AT_OWNER_WRITE | api::master::AuthorityType::AT_GROUP_READ | api::master::AuthorityType::AT_OTHER_READ);

		api::master::CreateFileResponse response;
		rpc::OfsRpcController controller(-1);

		_service->Create(&controller, &request, &response, nullptr);

		if (!controller.Failed()) {
			if (response.errcode() == api::master::ErrorCode::EC_NONE) {
				Node* node = GetNode(_root, realPath.c_str());
				if (node)
					node->expand = false;
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
		std::string parentPath = FindParentPathWithReal(realPath);

		api::master::RemoveRequest request;
		request.set_token(_token);
		request.set_path(realPath);

		api::master::RemoveResponse response;
		rpc::OfsRpcController controller(-1);

		_service->Remove(&controller, &request, &response, nullptr);

		if (!controller.Failed()) {
			if (response.errcode() == api::master::ErrorCode::EC_NONE) {
				Node* node = GetNode(_root, parentPath.c_str());
				if (node)
					node->expand = false;
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
			DisplayPath(_currentPath.c_str());
		}
	}

	void Client::Get(int32_t argc, char** argv) {
		args::ArgumentParser parser("This is a ofs get program.", "");
		args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
		args::Positional<std::string> path(parser, "path", "path");
		args::Positional<std::string> local(parser, "local", "local path");

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

		FileDownloader downloader(_service, _token);
		downloader.Start(realPath, args::get(local));
	}

	int32_t Client::Expand(const char * path) {
		Node * node = GetNode(_root, path);
		if (node && !node->dir) {
			return false;
		}

		if (!node || !node->expand || node->expandExpireTick < olib::GetTickCount()) {
			api::master::ListRequest request;
			request.set_token(_token);
			request.set_path(path);

			api::master::ListResponse response;
			rpc::OfsRpcController controller(-1);

			_service->List(&controller, &request, &response, nullptr);

			if (!controller.Failed()) {
				if (!node)
					node = CreateNode(_root, path);

				if (response.errcode() == api::master::ErrorCode::EC_NONE) {			
					for (int32_t i = 0; i < (int32_t)response.files_size(); ++i) {
						const auto& file = response.files(i);

						auto itr = std::find_if(node->children.begin(), node->children.end(), [&file](Node * n) {
							return n->name == file.name();
						});

						Node* child = nullptr;
						if (itr == node->children.end()) {
							child = new Node;

							child->expand = false;
							node->children.emplace_back(child);
						}
						else
							child = *itr;

						child->name = file.name();
						child->owner = file.owner();
						child->ownerGroup = file.group();
						child->authority = file.authority();
						child->dir = file.dir();
						child->createTime = file.createtime();
						child->updateTime = file.updatetime();
						child->size = file.size();
					}

					node->expand = true;
					node->expandExpireTick = olib::GetTickCount() + EXPAND_EXPIRE_INTERVAL;
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

	std::string ToAuthStr(int16_t authority, bool dir) {
		std::string ret;
		ret += dir ? 'd' : '-';

		if (authority & api::master::AuthorityType::AT_OWNER_READ)
			ret += 'r';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_OWNER_WRITE)
			ret += 'w';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_OWNER_EXECUTE)
			ret += 'x';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_GROUP_READ)
			ret += 'r';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_GROUP_WRITE)
			ret += 'w';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_GROUP_EXECUTE)
			ret += 'x';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_OTHER_READ)
			ret += 'r';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_OTHER_WRITE)
			ret += 'w';
		else
			ret += '-';

		if (authority & api::master::AuthorityType::AT_OTHER_EXECUTE)
			ret += 'x';
		else
			ret += '-';

		return ret;
	}

	void Client::DisplayPath(const char* path) {
		Node* node = GetNode(_root, path);
		if (node && !node->dir)
			return;

		if (node && node->expand) {
			for (auto* child : node->children)
				printf("%s\t%s\t%s\t%d\t%s\t%s\n", ToAuthStr(child->authority, child->dir).c_str(), child->owner.c_str(), child->ownerGroup.c_str(), child->dir ? 1024 : child->size, olib::FomateTimeStamp(child->updateTime).c_str(), child->name.c_str());
		}
	}

	std::string Client::FindReal(const std::string& path) {
		if (path.at(0) != '/')
			return FindReal(_currentPath + "/" + path);
			
		std::vector<std::string> units = Split(path, "/");
		std::vector<std::string> stacks;
		for (auto& str : units) {
			if (str == "..") {
				if (stacks.empty())
					return FindParentPathWithReal(_currentPath);

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

	std::string Client::FindParentPathWithReal(const std::string& path) {
		std::string real = path;

		if (!real.empty()) {
			auto pos = real.find_last_of('/');
			if (pos != std::string::npos)
				real.erase(pos);
		}
		return real;
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
