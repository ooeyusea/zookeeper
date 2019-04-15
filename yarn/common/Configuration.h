#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__
#include "hnet.h"
#include "XmlReader.h"

namespace yarn {
	class YarnNodeManagerConfiguration {
	public:
		YarnNodeManagerConfiguration() {}
		~YarnNodeManagerConfiguration() {}

		bool Initialize(const olib::IXmlObject& object);

		inline const std::string& GetName() const { return _name; }
		inline const std::string& GetIp() const { return _ip; }
		inline int32_t GetPort() const { return _port; }

		inline int32_t GetHeartBeatInterval() const { return _heartBeatInterval; }

	private:
		std::string _name;
		std::string _ip;
		int32_t _port;

		int32_t _heartBeatInterval;
	};

	class YarResourceManagerConfiguration {
	public:
		YarResourceManagerConfiguration() {}
		~YarResourceManagerConfiguration() {}

		bool Initialize(const olib::IXmlObject& object);

		inline const std::string& GetIp() const { return _ip; }
		inline int32_t GetPort() const { return _port; }

	private:
		std::string _ip;
		int32_t _port;
	};

	class YarnConfiguration {
	public:
		YarnConfiguration(bool rm) : _isRm(rm) {}
		~YarnConfiguration() {}

		bool LoadFrom(const char * path);

		inline const YarnNodeManagerConfiguration& GetNodeManager() const { return _node; }
		inline const YarResourceManagerConfiguration& GetResourceManager() const { return _resouce; }

	private:
		bool _isRm = false;

		YarnNodeManagerConfiguration _node;
		YarResourceManagerConfiguration _resouce;
	};
}

#endif //__CONFIGURATION_H__
