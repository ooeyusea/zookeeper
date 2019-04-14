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

		const std::string& GetName() const { return _name; }
		const std::string& GetIp() const { return _ip; }
		int32_t GetPort() const { return _port; }

	private:
		std::string _name;
		std::string _ip;
		int32_t _port;
	};

	class YarnConfiguration {
	public:
		YarnConfiguration(bool rm) : _isRm(rm) {}
		~YarnConfiguration() {}

		bool LoadFrom(const char * path);

		const YarnNodeManagerConfiguration& GetNodeManager() const { return _node; }

	private:
		bool _isRm = false;
		YarnNodeManagerConfiguration _node;
	};
}

#endif //__CONFIGURATION_H__
