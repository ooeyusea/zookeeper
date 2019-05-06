#ifndef __COMMON_INSTRUCTION_SEQUENCE_H__
#define __COMMON_INSTRUCTION_SEQUENCE_H__
#include "hnet.h"

namespace ofs {
	namespace instruction_sequence {
		struct InstructionSequenceExecutor {
			virtual ~InstructionSequenceExecutor() {}

			virtual void Execute(void * p) = 0;
		};

		class OfsInstructionSequence {
			struct Chunk {
				InstructionSequenceExecutor * executor;
			};
		public:
			OfsInstructionSequence() {}
			~OfsInstructionSequence() {}

			template <typename DataType>
			inline void Push(InstructionSequenceExecutor * executor, DataType && data) {
				Chunk * p = (Chunk *)malloc(sizeof(Chunk) + sizeof(DataType));
				p->executor = executor;

				DataType * target = new (p + 1) DataType(std::forward<DataType>(data));
				
				try {
					_queue << p;
				}
				catch (hn_channel_close_exception&) {
					target->~DataType();
					free(p);
				}
			}

			bool Start();
			void Stop();

		private:
			void Process();

		private:
			std::unordered_map<int32_t *, InstructionSequenceExecutor*> _executors;

			hn_channel<void *> _queue;
		};

		template <typename T>
		class TExecutor : public InstructionSequenceExecutor {
		public:
			TExecutor() {}
			virtual ~TExecutor() {}

			inline void AttachTo(OfsInstructionSequence * is) { _is = is; }

			inline void Push(T&& t) {
				_is->Push(this, std::forward<T>(t));
			}

			virtual void Execute(void * p) {
				T * t = static_cast<T*>(p);
				Execute(*t);
				t->~T();
			}

		protected:
			virtual void Execute(T& t) = 0;
			
		protected:
			OfsInstructionSequence * _is;
		};
	}
}

#endif //__COMMON_INSTRUCTION_SEQUENCE_H__
