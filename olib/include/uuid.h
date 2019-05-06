#ifndef __UUID_H__
#define __UUID_H__
#include "hnet.h"
#include "time_helper.h"

namespace olib {
	namespace uuid {
		class UUID {
		public:
			UUID() : _high(0), _low(0) {}
			UUID(int64_t high, int64_t low) : _high(high), _low(low) {}
			~UUID() {}

			inline bool operator<(const UUID& rhs) const {
				return _high < rhs._high || (_high == rhs._high && _low < rhs._low);
			}

			inline bool operator>(const UUID& rhs) const {
				return _high > rhs._high || (_high == rhs._high && _low > rhs._low);
			}

			inline bool operator!=(const UUID& rhs) const {
				return _high != rhs._high || _low != rhs._low;
			}

			inline bool operator==(const UUID& rhs) const {
				return _high == rhs._high && _low == rhs._low;
			}

			inline int64_t Hash() const {
				return _high & _low;
			}

			inline static UUID Generate() {
				static int64_t s_tick = GetTimeStamp();
				static int64_t s_next = 1;

				int64_t now = GetTimeStamp();
				if (now != s_tick) {
					s_tick = now;
					s_next = 1;
				}

				return { s_tick, (int64_t)(g_id << 32 | s_next) };
			}

			inline static UUID Zero() { return {0, 0}; }

			inline static void SetId(int64_t id) { g_id = id; }

			template <typename AR>
			inline void Archive(AR& ar) {
				ar & _high;
				ar & _low;
			}

			inline int64_t GetHigh() const { return _high; }
			inline int64_t GetLow() const { return _low; }

		private:
			int64_t _high;
			int64_t _low;

			static int64_t g_id;
		};
	}
}

namespace std {
	template <>
	struct hash<olib::uuid::UUID> {
		inline size_t operator()(const olib::uuid::UUID& key) const {
			return key.Hash();
		}
	};
}

#endif //__UUID_H__
