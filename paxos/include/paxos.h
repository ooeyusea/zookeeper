#ifndef __PAXOS_H__
#define __PAXOS_H__
#include "hnet.h"

namespace paxos {
	struct IDataSet {
		virtual ~IDataSet() {}

		virtual bool LoadFromFile(const std::string& path) = 0;
		virtual bool SaveToFile(const std::string& path) = 0;

		virtual void BuildFromData(const std::string& data) = 0;
		virtual void GetData(std::string& data) = 0;
	};

	struct ITransaction {
		virtual ~ITransaction() {}

		virtual void Apply(IDataSet * dataset, hn_iachiver& ar) = 0;
		virtual void Rollback(IDataSet * dataset, hn_iachiver& ar) = 0;
	};

	template <typename T>
	struct Function;

	template <typename T, typename R, typename... Args>
	struct Function<R (T::*)(Args...)> {
		inline void Invoke(T& t, IDataSet * dataset, hn_iachiver& ar) {

		}
	};

	template <typename T, typename... Args>
	struct Function<void (T::*)(Args...)> {
		inline void Invoke(T& t, IDataSet * dataset, hn_iachiver& ar) {

		}
	};

	template <typename F>
	Function<F> GetFunction(F f) {
		return Function<F>(f);
	}

	struct ITransactionParser {
		virtual ~ITransactionParser() {}

		virtual ITransaction * Parse(const std::string& log) = 0;
	};

	struct IAction {
		virtual ~IAction() {}

		virtual ITransaction * BuildTransaction() = 0;
	};

	class Paxos {
	public:
		Paxos() {}
		~Paxos() {}

		bool Start(const std::string& path);

		void DoAction(IAction* action);

	private:

	};
}

#endif //__PAXOS_H__
