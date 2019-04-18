#ifndef __PAXOS_H__
#define __PAXOS_H__
#include "hnet.h"
#include "function_helper.h"

namespace paxos {
	struct IStateData {
		virtual ~IStateData() {}
		virtual void Release() = 0;

		virtual bool LoadFromFile(const std::string& path) = 0;
		virtual bool SaveToFile(const std::string& path) = 0;

		virtual void BuildFromData(const std::string& data) = 0;
		virtual void GetData(std::string& data) = 0;
	};

	class ITransaction {
	public:
		ITransaction(const int32_t type) : _type(type) {}
		virtual ~ITransaction() {}

		virtual void Apply(IStateData * dataset, hn_iachiver& ar) = 0;
		virtual void Rollback(IStateData * dataset, hn_iachiver& ar) = 0;

		inline int32_t GetType() const { return _type; }

	protected:
		int32_t _type;
	};

	template <bool> struct BoolTraits {};

	template <typename T> struct IsVoid { static const bool value = false; };
	template <> struct IsVoid<void> { static const bool value = true; };

	template <typename T>
	struct TranscationActionType {
		typedef decltype(olib::GetReturnType(&T::DoApply)) return_type;
		typedef decltype(olib::Self2Self(&T::DoApply)) action_type;
	};

	template <typename T, int32_t logType>
	class Transcation : public ITransaction {
		typename typename TranscationActionType<T>::return_type result_type;
	public:
		Transcation() : ITransaction(logType) {}
		virtual ~Transcation() {}

		virtual void Apply(IStateData * dataset, hn_iachiver& ar) {
			Apply(dataset, ar, BoolTraits<ret>(), BoolTraits<IsVoid<result_type>>());
		}

		virtual void Rollback(IStateData * dataset, hn_iachiver& ar) {
			olib::Callback<IStateData*>::Deal(ar, *static_cast<T*>(this), &T::DoRollback, dataset);
		}

		inline void SetResult(result_type * result) { _result = result; }

	protected:
		inline void Apply(IStateData * dataset, hn_iachiver& ar, BoolTraits<true>, BoolTraits<true>) {
			if (_result)
				*_result = olib::Callback<IStateData*>::Deal(ar, *static_cast<T*>(this), &T::DoApply, dataset);
			else
				olib::Callback<IStateData*>::Deal(ar, *static_cast<T*>(this), &T::DoApply, dataset);
		}

		inline void Apply(IStateData * dataset, hn_iachiver& ar, BoolTraits<true>, BoolTraits<false>) {
			olib::Callback<IStateData*>::Deal(ar, *static_cast<T*>(this), &T::DoApply, dataset);
		}

		template <typename A>
		inline void Apply(IStateData * dataset, hn_iachiver& ar, BoolTraits<false>, A) {
			olib::Callback<IStateData*>::Deal(ar, *static_cast<T*>(this), &T::DoApply, dataset);
		}

	protected:
		result_type * _result = nullptr;
	};

	struct IPaxosImpl {
		virtual ~IPaxosImpl() {}

		virtual bool Start(IStateData * data, const std::string& path) = 0;

		virtual void RegisterLogType(ITransaction * transaction) = 0;
		virtual void DoTransaction(ITransaction * transaction, const char * param, int32_t size) = 0;
	};

	class Paxos {
	public:
		template <int32_t size, typename T, typename R, typename... FixArgs>
		struct PaxosDo {
			Paxos& p;
			PaxosDo(Paxos& p_) : p(p_) {}

			template <typename... Args>
			inline R Do(Args... args) {
				char buff[size];
				hn_ostream stream(buff, size);
				hn_oachiver ar(stream, 0);

				R r;
				T t;
				t.SetResult(&r);
				ar << t.GetType();
				olib::ParamPacker<IStateData*>::Dealer<typename TranscationActionType<T>::action_type>::invoke(ar, args...);

				p.DoTransaction(&t, buff, (int32_t)stream.size());
				return ret;
			}
		};

		template <int32_t size, typename T, typename... FixArgs>
		struct PaxosDo<size, T, void, FixArgs...> {
			Paxos& p;
			PaxosDo(Paxos& p_) : p(p_) {}

			template <typename... Args>
			inline void Do(Args... args) {
				char buff[size];
				hn_ostream stream(buff, size);
				hn_oachiver ar(stream, 0);

				T t;
				ar << t.GetType();
				olib::ParamPacker<IStateData*>::Dealer<typename TranscationActionType<T>::action_type>::invoke(ar, args...);

				p.DoTransaction(&t, buff, (int32_t)stream.size());
			}
		};

		Paxos();
		~Paxos() {}

		inline Paxos& RegisterLogType(ITransaction * transaction) {
			_impl->RegisterLogType(transaction);
			return *this;
		}

		template <int32_t size, typename T>
		PaxosDo<size, T, typename TranscationActionType<T>::result_type, IStateData *> GetPaxosDo{
			return PaxosDo<size, T, typename TranscationActionType<T>::result_type, IStateData *>(*this);
		}

		inline bool Start(IStateData * data, const std::string& path) {
			return _impl->Start(data, path);
		}

		inline void DoTransaction(ITransaction * transaction, const char * param, int32_t size) {
			_impl->DoTransaction(transaction, param, size);
		}

	private:
		IPaxosImpl * _impl = nullptr;
	};
}

#endif //__PAXOS_H__
