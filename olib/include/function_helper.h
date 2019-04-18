#ifndef __FUNCTION_HELPER_H__
#define __FUNCTION_HELPER_H__
#include "hnet.h"

namespace olib {
	template <typename F>
	struct FunctionReturnType;

	template <typename R, typename... ParsedArgs>
	struct FunctionReturnType<R(*)(ParsedArgs...)> {
		typedef R type;
	};

	template <typename T, typename R, typename... ParsedArgs>
	struct FunctionReturnType<R(T::*)(ParsedArgs...)> {
		typedef R type;
	};

	template <typename R, typename... ParsedArgs>
	R GetReturnType(R(*)(ParsedArgs...)) {

	}

	template <typename T, typename R, typename... ParsedArgs>
	R GetReturnType(R(T::*)(ParsedArgs...)) {

	}

	template <typename T>
	T Self2Self(T) {

	}

	template <typename... FixArgs>
	struct Callback {
		template <typename R, typename... ParsedArgs>
		struct Dealer {
			static inline R invoke(hn_iachiver & reader, R(*fn)(FixArgs..., ParsedArgs...), FixArgs... fixArgs, ParsedArgs... args) {
				return fn(fixArgs..., args...);
			}

			template <typename T, typename... Args>
			static inline R invoke(hn_iachiver & reader, R(*fn)(FixArgs..., ParsedArgs..., T, Args...), FixArgs... fixArgs, ParsedArgs... args) {
				T t;
				reader >> t;
				if (reader.Fail()) {
					throw std::logic_error("not enough param");
				}

				return Dealer<ParsedArgs..., T>::invoke(reader, fn, fixArgs..., args..., t);
			}

			template <typename C>
			static inline R invoke(hn_iachiver & reader, C & c, R(C::*fn)(FixArgs..., ParsedArgs...), FixArgs... fixArgs, ParsedArgs... args) {
				return (c.*fn)(fixArgs..., args...);
			}

			template <typename C, typename T, typename... Args>
			static inline R invoke(hn_iachiver & reader, C & c, R(C::*fn)(FixArgs..., ParsedArgs..., T, Args...), FixArgs... fixArgs, ParsedArgs... args) {
				std::remove_const_t<std::remove_reference_t<T>> t;
				reader >> t;
				if (reader.Fail()) {
					throw std::logic_error("not enough param");
				}

				return Dealer<ParsedArgs..., T>::invoke(reader, c, fn, fixArgs..., args..., t);
			}

			template <typename C>
			static inline R invoke(hn_iachiver & reader, const C & c, R(C::*fn)(FixArgs..., ParsedArgs...) const, FixArgs... fixArgs, ParsedArgs... args) {
				return (c.*fn)(fixArgs..., args...);
			}

			template <typename C, typename T, typename... Args>
			static inline void R(hn_iachiver & reader,const C & c, R(C::*fn)(FixArgs..., ParsedArgs..., T, Args...) const, FixArgs... fixArgs, ParsedArgs... args) {
				std::remove_const_t<std::remove_reference_t<T>> t;
				reader >> t;
				if (reader.Fail()) {
					throw std::logic_error("not enough param");
				}

				return Dealer<ParsedArgs..., T>::invoke(reader, c, fn, fixArgs..., args..., t);
			}
		};

		template <typename... ParsedArgs>
		struct Dealer<void, ParsedArgs...> {
			static inline void invoke(hn_iachiver & reader, void (*fn)(FixArgs..., ParsedArgs...), FixArgs... fixArgs, ParsedArgs... args) {
				fn(fixArgs..., args...);
			}

			template <typename T, typename... Args>
			static inline void invoke(hn_iachiver & reader, void (*fn)(FixArgs..., ParsedArgs..., T, Args...), FixArgs... fixArgs, ParsedArgs... args) {
				T t;
				reader >> t;
				if (reader.Fail()) {
					throw std::logic_error("not enough param");
				}

				Dealer<ParsedArgs..., T>::invoke(reader, fn, fixArgs..., args..., t);
			}

			template <typename C>
			static inline void invoke(hn_iachiver & reader, C & c, void (C::*fn)(FixArgs..., ParsedArgs...), FixArgs... fixArgs, ParsedArgs... args) {
				(c.*fn)(fixArgs..., args...);
			}

			template <typename C, typename T, typename... Args>
			static inline void invoke(hn_iachiver & reader, C & c, void (C::*fn)(FixArgs..., ParsedArgs..., T, Args...), FixArgs... fixArgs, ParsedArgs... args) {
				std::remove_const_t<std::remove_reference_t<T>> t;
				reader >> t;
				if (reader.Fail()) {
					throw std::logic_error("not enough param");
				}

				Dealer<ParsedArgs..., T>::invoke(reader, c, fn, fixArgs..., args..., t);
			}

			template <typename C>
			static inline void invoke(hn_iachiver & reader, const C & c, void (C::*fn)(FixArgs..., ParsedArgs...) const, FixArgs... fixArgs, ParsedArgs... args) {
				(c.*fn)(fixArgs..., args...);
			}

			template <typename C, typename T, typename... Args>
			static inline void invoke(hn_iachiver & reader, const C & c, void (C::*fn)(FixArgs..., ParsedArgs..., T, Args...) const, FixArgs... fixArgs, ParsedArgs... args) {
				std::remove_const_t<std::remove_reference_t<T>> t;
				reader >> t;
				if (reader.Fail()) {
					throw std::logic_error("not enough param");
				}

				Dealer<ParsedArgs..., T>::invoke(reader, c, fn, fixArgs..., args..., t);
			}
		};

		template <typename R, typename... Args>
		static inline R Deal(hn_iachiver & reader, R(*fn)(FixArgs..., Args...), FixArgs... fixArgs) {
			return Dealer<>::invoke(reader, fn, fixArgs...);
		}

		template <typename R>
		static inline R Deal(hn_iachiver & reader, R(*fn)(FixArgs...), FixArgs... fixArgs) {
			return fn(fixArgs...);
		}

		template <typename R, typename C, typename... Args>
		static inline R Deal(hn_iachiver & reader,  C & c, R(C::*fn)(FixArgs..., Args...), FixArgs... fixArgs) {
			return Dealer<>::invoke(reader, c, fn, fixArgs...);
		}

		template <typename R, typename C>
		static inline R Deal(hn_iachiver & reader, C & c, R(C::*fn)(FixArgs...), FixArgs... fixArgs) {
			return (c.*fn)(fixArgs...);
		}

		template <typename R, typename C, typename... Args>
		static inline R Deal(hn_iachiver & reader, const C & c, R(C::*fn)(FixArgs..., Args...) const, FixArgs... fixArgs) {
			return Dealer<>::invoke(reader, c, fn, fixArgs...);
		}

		template <typename R, typename C>
		static inline R Deal(hn_iachiver& reader, const C & c, R(C::*fn)(FixArgs...) const, FixArgs... fixArgs) {
			return (c.*fn)(fixArgs...);
		}

		template <typename... Args>
		static inline void Deal(hn_iachiver & reader, void (*fn)(FixArgs..., Args...), FixArgs... fixArgs) {
			Dealer<>::invoke(reader, fn, fixArgs...);
		}

		static inline void Deal(hn_iachiver & reader, void (*fn)(FixArgs...), FixArgs... fixArgs) {
			fn(fixArgs...);
		}

		template <typename C, typename... Args>
		static inline void Deal(hn_iachiver & reader, C & c, void (C::*fn)(FixArgs..., Args...), FixArgs... fixArgs) {
			Dealer<>::invoke(reader, c, fn, fixArgs...);
		}

		template <typename C>
		static inline void Deal(hn_iachiver & reader, C & c, void (C::*fn)(FixArgs...), FixArgs... fixArgs) {
			(c.*fn)(fixArgs...);
		}

		template <typename C, typename... Args>
		static inline void Deal(hn_iachiver & reader, const C & c, void (C::*fn)(FixArgs..., Args...) const, FixArgs... fixArgs) {
			Dealer<>::invoke(reader, c, fn, fixArgs...);
		}

		template <typename R, typename C>
		static inline void Deal(hn_iachiver& reader, const C & c, void (C::*fn)(FixArgs...) const, FixArgs... fixArgs) {
			(c.*fn)(fixArgs...);
		}
	};

	template <typename... Args>
	struct ArgsPacker {};

	template <>
	struct ArgsPacker<> {
		static inline void invoke(hn_oachiver & writer) {}
	};

	template <typename T, typename... Args>
	struct ArgsPacker<T, Args...> {
		static inline void invoke(hn_oachiver & writer, T t, Args... args) {
			writer << t;
			ArgsPacker<Args...>::invoke(writer, args...);
		}
	};

	template <typename... FixArgs>
	struct ParamPacker {
		template <typename F>
		struct Dealer;

		template <typename R, typename... Args>
		struct Dealer<R(*)(FixArgs..., Args...)> {
			static inline void invoke(hn_oachiver & writer, Args... args) {
				ArgsPacker<Args...>::invoke(writer, args...);
			}
		};

		template <typename T, typename R, typename... Args>
		struct Dealer<R(T::*)(FixArgs..., Args...)> {
			static inline void invoke(hn_oachiver & writer, Args... args) {
				ArgsPacker<Args...>::invoke(writer, args...);
			}
		};

		template <typename T, typename R, typename... Args>
		struct Dealer<R(T::*)(FixArgs..., Args...) const> {
			static inline void invoke(hn_oachiver & writer, Args... args) {
				ArgsPacker<Args...>::invoke(writer, args...);
			}
		};
	};
}

#endif //__FUNCTION_HELPER_H__
