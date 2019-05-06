#ifndef __OFS_MASTER_H__
#define __OFS_MASTER_H__
#include "hnet.h"
#include "singleton.h"
#include "instruction_sequence/InstructionSequence.h"

namespace ofs {
	class Master : public olib::Singleton<Master> {
	public:
		Master();
		virtual ~Master() {}

		bool Start(const std::string& path);

	private:
		instruction_sequence::OfsInstructionSequence _is;
	};
}

#endif //__OFS_MASTER_H__
