#include "InstructionSequence.h"

namespace ofs {
	namespace instruction_sequence {
		bool OfsInstructionSequence::Start() {
			hn_fork[this]{
				Process();
			};

			return true;
		}

		void OfsInstructionSequence::Stop() {
			_queue.Close();

			hn_sleep 500;
		}

		void OfsInstructionSequence::Process() {
			while (true) {
				void * data = nullptr;
				try {
					_queue >> data;
				}
				catch (hn_channel_close_exception&) {
					break;
				}

				Chunk * p = (Chunk *)data;
				p->executor->Execute(p + 1);

				free(data);
			}
		}
	}
}
