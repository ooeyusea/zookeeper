#ifndef __OFS_CLIENT_H__
#define __OFS_CLIENT_H__
#include "hnet.h"
#include "rpc/Rpc.h"
#include "api/OfsMaster.pb.h"

namespace ofs {
	class Client {
		typedef void (Client::*CommandDealFuncType)(int32_t argc, char ** argv);
	public:
		Client();
		virtual ~Client() {}

		bool Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password);

		void DoCommand(const std::string& line);
		void Slash();

	private:
		bool Login(const std::string& username, const std::string& password);

		void ChangeDir(const std::string& path);
		void Mkdir(const std::string& path);
		void Touch(const std::string& path);
		void Remove(const std::string& path);
		void List(int32_t argc);

	private:
		ofs::rpc::OfsRpcChannel _channel;
		api::OfsFileService * _service = nullptr;

		std::string _token;
		bool _supper;
		std::string _username;
		std::string _host;

		std::unordered_map<std::string, CommandDealFuncType> _commands;

		std::string _currentPath;
	};
}

#endif //__OFS_MASTER_H__
