#include "transaction.h"
#include "TestStateData.h"

GET_DLL_ENTRANCE;

extern "C" ZK_EXPORT IStateDataFactory * GET_FACTORY_FN() {
	return new test::TestStateDataFactory;
}
