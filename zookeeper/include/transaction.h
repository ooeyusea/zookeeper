#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__
#include "hnet.h"

struct IStateData {
	virtual ~IStateData() {}

	virtual void Release() = 0;

	virtual bool LoadFromFile(const std::string& path) = 0;
	virtual bool SaveToFile(const std::string& path) = 0;

	virtual bool PreCheck(const std::string& data) = 0;
	virtual void Apply(const std::string& data) = 0;
	virtual void Rollback(const std::string& data) = 0;

	virtual void BuildFromData(const std::string& data) = 0;
	virtual void GetData(std::string& data) = 0;

	virtual std::string Read(const std::string& data) = 0;
};

struct IStateDataFactory {
	virtual ~IStateDataFactory() {}

	virtual IStateData * Create() = 0;
};

typedef IStateDataFactory * (*GetFactoryFn)(void);
#define NAME_OF_GET_FACTORY_FN "GetFactory"
#define GET_FACTORY_FN GetFactory

#ifdef WIN32
#define GET_DLL_ENTRANCE \
	BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {\
		return TRUE;\
	}
#define ZK_EXPORT __declspec(dllexport)
#else
#define GET_DLL_ENTRANCE
#define ZK_EXPORT
#endif

#pragma pack(push, 1)

namespace transaction_def {
	struct ServiceHeader {
		int32_t size;
		int32_t op;
	};

	enum {
		OP_PROPOSE,
		OP_PROPOSE_ACK,
		OP_READ,
		OP_READ_RESULT,
		OP_PING,
		OP_PONG,
	};
}

#pragma pack(pop)
#endif //__TRANSACTION_H__
