#ifndef __OFS_CLIENT_H__
#define __OFS_CLIENT_H__
#include "hnet.h"
#include "rpc/Rpc.h"
#include "api/OfsMaster.pb.h"

namespace ofs {
	class Client {
		typedef void (Client::*CommandDealFuncType)(int32_t argc, char ** argv);

		struct Node {
			~Node() {
				for (auto* child : children) {
					delete child;
				}
			}

			std::string name;
			std::string owner;
			std::string ownerGroup;
			int16_t authority = 0;
			bool dir;
			int64_t createTime = 0;
			int64_t updateTime = 0;
			int32_t size = 0;
			bool expand = false;
			int64_t expandExpireTick = 0;

			std::vector<Node*> children;
		};

	public:
		Client();
		virtual ~Client() {}

		bool Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password);

		void DoCommand(const std::string& line);
		void Slash();

		inline bool IsOpen() const { return _channel.IsOpen(); }

	private:
		bool Login(const std::string& username, const std::string& password);
		bool ReadRootPath();

		void ChangeDir(int32_t argc, char ** argv);
		void Mkdir(int32_t argc, char ** argv);
		void Touch(int32_t argc, char ** argv);
		void Remove(int32_t argc, char ** argv);
		void List(int32_t argc, char ** argv);
		void Put(int32_t argc, char** argv);
		void Get(int32_t argc, char** argv);

		int32_t Expand(const char * path);
		void DisplayPath(const char* path);
		int32_t GetSize(const std::string& path, int32_t* blockSize);
		bool CreateRemoteFile(const std::string& path);

		std::string FindReal(const std::string& path);
		std::tuple<std::string, std::string> FindRealWithName(const std::string& path);
		std::string FindParentPathWithReal(const std::string& path);

		Node * GetNode(Node& node, const char * path);
		Node * CreateNode(Node& node, const char * path);

		inline std::tuple<std::string, const char*> FetchSubPath(const char * path) {
			const char * slash = strchr(path, '/');
			if (slash)
				return std::make_tuple(std::string(path, slash - path), slash + 1);

			return std::make_tuple(std::string(path), nullptr);
		}

	private:
		ofs::rpc::OfsRpcChannel _channel;
		api::master::OfsFileService * _service = nullptr;

		std::string _token;
		bool _supper;
		std::string _username;
		std::string _host;

		std::unordered_map<std::string, CommandDealFuncType> _commands;

		std::string _currentPath;
		Node _root;
	};
}

#endif //__OFS_MASTER_H__
