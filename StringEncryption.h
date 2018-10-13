#pragma once
#include <string>
#include <cstdint>

#pragma warning( disable : 4244)
#pragma warning( disable : 4146)

namespace Crypto
{
	namespace Helper
	{
		constexpr size_t strlen_c(const char* str)
		{
			return *str ? 1 + strlen_c(str + 1) : 0;
		}
	}

	namespace FNV1
	{
		namespace impl
		{
			constexpr uint64_t fnv1a(uint64_t h, const char* s)
			{
				return (*s == 0) ? h : fnv1a((h ^ static_cast<uint64_t>(*s)) * 1099511628211ull, s + 1);
			}
		}

		constexpr uint64_t fnv1a(const char* s)
		{
			return impl::fnv1a(14695981039346656037ull, s);
		}
	}

	namespace pcg
	{
		namespace impl
		{
			constexpr uint64_t pcg32_advance(uint64_t s)
			{
				return s * 6364136223846793005ULL + (1442695040888963407ULL | 1);
			}

			constexpr uint64_t pcg32_advance(uint64_t s, int n)
			{
				return n == 0 ? s : pcg32_advance(pcg32_advance(s), n - 1);
			}

			constexpr uint32_t pcg32_xorshift(uint64_t s)
			{
				return ((s >> 18u) ^ s) >> 27u;
			}

			constexpr uint32_t pcg32_rot(uint64_t s)
			{
				return s >> 59u;
			}

			constexpr uint32_t pcg32_output(uint64_t s)
			{
				return (pcg32_xorshift(s) >> pcg32_rot(s))
					| (pcg32_xorshift(s) << ((-pcg32_rot(s)) & 31));
			}

			constexpr int BIT_DEPTH = 6;
			constexpr int MAX = (1 << BIT_DEPTH);
			constexpr int BIT_MASK = MAX - 1;

			template <uint64_t S, int H, int L>
			struct flag1
			{
				friend constexpr int adl_flag1(flag1<S, H, L>);
			};

			template <uint64_t S, int H>
			struct flag2
			{
				friend constexpr int adl_flag2(flag2<S, H>);
			};

			template <uint64_t S, int H>
			struct r1
			{
				template <int L, int = adl_flag1(flag1<S, H, L>{})>
				static constexpr int reader(int, flag1<S, H, L>)
				{
					return L;
				}

				template <int L>
				static constexpr int reader(
					float, flag1<S, H, L>, int R = reader(0, flag1<S, H, L - 1>{}))
				{
					return R;
				}

				static constexpr int reader(float, flag1<S, H, 0>)
				{
					return 0;
				}
			};

			template <uint64_t S>
			struct r2
			{
				template <int H, int = adl_flag2(flag2<S, H>{})>
				static constexpr int reader(int, flag2<S, H>)
				{
					return H;
				}

				template <int H>
				static constexpr int reader(
					float, flag2<S, H>, int R = reader(0, flag2<S, H - 1>{}))
				{
					return R;
				}

				static constexpr int reader(float, flag2<S, 0>)
				{
					return 0;
				}
			};

			template <uint64_t S, int H, int L>
			struct writelo
			{
				friend constexpr int adl_flag1(flag1<S, H, L>)
				{
					return L;
				}

				static constexpr int value = L;
			};

			template <uint64_t S, int H, bool B>
			struct writehi
			{
				friend constexpr int adl_flag2(flag2<S, H>)
				{
					return H;
				}

				static constexpr int value = H;
			};

			template <uint64_t S, int H>
			struct writehi<S, H, false>
			{
				static constexpr int value = H;
			};

			template <uint64_t S, int H, int L>
			struct writer
			{
				static constexpr int hi_value =
					writehi<S, H + 1, L == MAX>::value;
				static constexpr int lo_value =
					writelo<S, H, (L & BIT_MASK)>::value;
				static constexpr uint32_t value =
					pcg32_output(pcg32_advance(S, (H << BIT_DEPTH) + L));
			};
		}

		template <uint64_t S,
			int N = 1,
			int H = impl::r2<S>::reader(0, impl::flag2<S, impl::MAX - 1>{}),
			int L = impl::r1<S, H>::reader(0, impl::flag1<S, H, impl::MAX>{})>
			constexpr uint32_t pcg32(
				uint32_t R = impl::writer<S, H, L + N>::value)
		{
			return R;
		}
	}

	namespace Str
	{
		namespace detail
		{
			template <uint64_t S>
			constexpr char encrypt_at(const char* s, size_t idx)
			{
				return s[idx] ^
					static_cast<char>(pcg::impl::pcg32_output(pcg::impl::pcg32_advance(S, idx + 1)) >> 24);
			}
			template <size_t N>
			struct char_array
			{
				char data[N];
			};
			inline std::string decrypt(uint64_t S, const char* s, size_t n)
			{
				std::string ret;
				ret.reserve(n);
				for (size_t i = 0; i < n; ++i)
				{
					S = pcg::impl::pcg32_advance(S);
					ret.push_back(s[i] ^ static_cast<char>(pcg::impl::pcg32_output(S) >> 24));
				}
				return ret;
			}
			template <uint64_t S, size_t ...Is>
			constexpr char_array<sizeof...(Is)> encrypt(const char* s, std::index_sequence<Is...>)
			{
				return { { encrypt_at<S>(s, Is)... } };
			}
		}
		template <uint64_t S, size_t N>
		class encrypted_string
		{
		public:
			__forceinline explicit constexpr encrypted_string(const char(&a)[N])
				: m_enc(detail::encrypt<S>(a, std::make_index_sequence<N - 1>())) { }

			static constexpr size_t size()
			{
				return N - 1;
			}

			operator std::string() const
			{
				return detail::decrypt(S, m_enc.data, N - 1);
			}

		private:
			const detail::char_array<N - 1> m_enc;
		};
		template <uint64_t S, size_t N>
		__forceinline constexpr encrypted_string<S, N> make_encrypted_string(const char(&s)[N])
		{
			return encrypted_string<S, N>(s);
		}
	}
}

#define XORSEED uint64_t{Crypto::FNV1::fnv1a(__FILE__ __DATE__ __TIME__) + __LINE__}
#define MAKEXOR Crypto::Str::make_encrypted_string<XORSEED>
#define enc_str(s) std::string(s)
#define enc_char(s) enc_str(s).c_str()
#define enc_wstr(s) std::wstring(enc_str(s).begin(), enc_str(s).end())
#define enc_wchar(s) enc_wstr(s).c_str()
#define FNV1(str) \
[&]() { \
constexpr uint64_t ret = Crypto::FNV1::fnv1a(str); \
return ret; \
}()
