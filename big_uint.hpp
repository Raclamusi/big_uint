#pragma once


#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>
#include <cstddef>
#include <cstdint>
#include <cctype>


template <std::size_t Bit>
class big_uint {
private:
	template <class T>
	static constexpr bool has_single_bit(T x) noexcept {
		if (x) while (!(x & 1)) x >>= 1;
		return x == 1;
	}
	
	template <class T>
	static constexpr std::size_t bit_width(T x) noexcept {
		std::size_t width = 0;
		while (x) {
			x >>= 1;
			++width;
		}
		return width;
	}
	
	static_assert(Bit >= 128, "'Bit' must be 128 or bigger");
	static_assert(has_single_bit(Bit), "'Bit' must be a power of 2");
	
	using bits_type = std::conditional_t<Bit == 128, std::uint64_t, big_uint<Bit / 2>>;
	bits_type ubits;
	bits_type lbits;
	
	constexpr big_uint(const bits_type& ubits, const bits_type& lbits) : ubits(ubits), lbits(lbits) {}
	
public:
	constexpr big_uint() : ubits(), lbits() {}
	
	big_uint(const big_uint&) = default;
	big_uint(big_uint&&) = default;
	big_uint& operator=(const big_uint&) = default;
	big_uint& operator=(big_uint&&) = default;
	~big_uint() = default;
	
	template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
	constexpr big_uint(T x) : ubits(), lbits(x) {
		if constexpr (std::is_signed_v<T>) {
			if (x < 0) ubits = ~ubits;
		}
	}
	constexpr big_uint(const char* x) : big_uint(std::string_view{x}) {}
	constexpr big_uint(const std::string_view &x) {
		*this = x;
	}
	template <std::size_t B>
	constexpr big_uint(const big_uint<B>& x) : ubits(x >> (Bit / 2)), lbits(x) {}
	
	template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
	constexpr big_uint& operator=(T x) {
		ubits = 0;
		if constexpr (std::is_signed_v<T>) {
			if (x < 0) ubits = ~ubits;
		}
		lbits = x;
		return *this;
	}
	constexpr big_uint& operator=(const char* x) {
		return *this = std::string_view{x};
	}
	constexpr big_uint& operator=(const std::string_view &x) {
		*this = 0;
		for (char c : x) {
			if (!std::isdigit(c)) throw std::invalid_argument("'x' is not unsigned integer");
			*this *= 10;
			*this += c - '0';
		}
		return *this;
	}
	template <std::size_t B>
	constexpr big_uint& operator=(const big_uint<B>& x) {
		ubits = x >> (Bit / 2);
		lbits = x;
		return *this;
	}
	
	constexpr big_uint operator+() const {
		return *this;
	}
	constexpr big_uint operator-() const {
		auto t = ~*this;
		++t;
		return std::move(t);
	}
	constexpr big_uint& operator++() & {
		++lbits;
		if (lbits == 0) ++ubits;
		return *this;
	}
	constexpr big_uint& operator--() & {
		if (lbits == 0) --ubits;
		--lbits;
		return *this;
	}
	constexpr big_uint operator++(int) & {
		auto t = *this;
		++*this;
		return std::move(t);
	}
	constexpr big_uint operator--(int) & {
		auto t = *this;
		--*this;
		return std::move(t);
	}
	constexpr big_uint& operator+=(const big_uint& x) & {
		if (lbits > ~bits_type{} - x.lbits) ++ubits;
		lbits += x.lbits;
		ubits += x.ubits;
		return *this;
	}
	template <class T>
	constexpr big_uint& operator+=(const T& x) & {
		return *this += big_uint(x);
	}
	constexpr big_uint& operator-=(const big_uint& x) & {
		if (lbits < x.lbits) --ubits;
		lbits -= x.lbits;
		ubits -= x.ubits;
		return *this;
	}
	template <class T>
	constexpr big_uint& operator-=(const T& x) & {
		return *this -= big_uint(x);
	}
	constexpr big_uint& operator*=(const big_uint& x) & {
		auto lu = lbits >> (Bit / 4);
		auto ru = x.lbits >> (Bit / 4);
		auto ll = lbits << (Bit / 4) >> (Bit / 4);
		auto rl = x.lbits << (Bit / 4) >> (Bit / 4);
		auto lurl = lu * rl;
		auto llru = ll * ru;
		auto lurlu = lurl >> (Bit / 4);
		auto llruu = llru >> (Bit / 4);
		auto lurll = std::move(lurl) << (Bit / 4);
		auto llrul = std::move(llru) << (Bit / 4);
		ubits *=  x.lbits;
		ubits += std::move(lbits) * x.ubits;
		ubits += std::move(lu) * ru;
		ubits += lurlu;
		ubits += llruu;
		lbits = std::move(ll) * rl;
		if (lbits > ~bits_type{} - lurll) ++ubits;
		lbits += lurll;
		if (lbits > ~bits_type{} - llrul) ++ubits;
		lbits += llrul;
		return *this;
	}
	template <class T>
	constexpr big_uint& operator*=(const T& x) & {
		return *this *= big_uint(x);
	}
	constexpr big_uint& operator/=(const big_uint& x) & {
		if (x == 0) throw std::invalid_argument("division by zero");
		const std::size_t n = sizeof(x) * 8 - bit_width(x);
		auto y = x << n;
		big_uint t = 0;
		for (std::size_t i = 0; i <= n; ++i) {
			t <<= 1;
			if (*this >= y) {
				*this -= y;
				t |= 1;
			}
			y >>= 1;
		}
		*this = std::move(t);
		return *this;
	}
	template <class T>
	constexpr big_uint& operator/=(const T& x) & {
		return *this /= big_uint(x);
	}
	constexpr big_uint& operator%=(const big_uint& x) & {
		if (x == 0) throw std::invalid_argument("division by zero");
		const std::size_t n = sizeof(x) * 8 - bit_width(x);
		auto y = x << n;
		for (std::size_t i = 0; i <= n; ++i) {
			if (*this >= y) *this -= y;
			y >>= 1;
		}
		return *this;
	}
	template <class T>
	constexpr big_uint& operator%=(const T& x) & {
		return *this %= big_uint(x);
	}
	
	constexpr big_uint& operator<<=(std::size_t x) & {
		if (x < Bit / 2) ubits <<= x;
		else ubits = 0;
		if (x > 0 && x < Bit) ubits |= x < Bit / 2 ? lbits >> (Bit / 2 - x) : lbits << (x - Bit / 2);
		if (x < Bit / 2) lbits <<= x;
		else lbits = 0;
		return *this;
	}
	constexpr big_uint& operator>>=(std::size_t x) & {
		if (x < Bit / 2) lbits >>= x;
		else lbits = 0;
		if (x > 0 && x < Bit) lbits |= x < Bit / 2 ? ubits << (Bit / 2 - x) : ubits >> (x - Bit / 2);
		if (x < Bit / 2) ubits >>= x;
		else ubits = 0;
		return *this;
	}
	constexpr big_uint operator~() const {
		return { ~lbits, ~ubits };
	}
	constexpr big_uint& operator&=(const big_uint& x) & {
		lbits &= x.lbits;
		ubits &= x.ubits;
		return *this;
	}
	template <class T>
	constexpr big_uint& operator&=(const T& x) & {
		return *this &= big_uint(x);
	}
	constexpr big_uint& operator|=(const big_uint& x) & {
		lbits |= x.lbits;
		ubits |= x.ubits;
		return *this;
	}
	template <class T>
	constexpr big_uint& operator|=(const T& x) & {
		return *this |= big_uint(x);
	}
	constexpr big_uint& operator^=(const big_uint& x) & {
		lbits ^= x.lbits;
		ubits ^= x.ubits;
		return *this;
	}
	template <class T>
	constexpr big_uint& operator^=(const T& x) & {
		return *this ^= big_uint(x);
	}
	
	constexpr explicit operator bool() const {
		return ubits || lbits;
	}
	constexpr bool operator!() const {
		return !operator bool();
	}
	
	template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
	constexpr explicit operator T() const {
		return static_cast<T>(lbits);
	}
	
	friend constexpr big_uint operator+(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs += rhs);
	}
	template <class T>
	friend constexpr big_uint operator+(big_uint lhs, const T& rhs) {
		return std::move(lhs) + big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator+(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) + rhs;
	}
	friend constexpr big_uint operator-(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs -= rhs);
	}
	template <class T>
	friend constexpr big_uint operator-(big_uint lhs, const T& rhs) {
		return std::move(lhs) - big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator-(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) - rhs;
	}
	friend constexpr big_uint operator*(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs *= rhs);
	}
	template <class T>
	friend constexpr big_uint operator*(big_uint lhs, const T& rhs) {
		return std::move(lhs) * big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator*(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) * rhs;
	}
	friend constexpr big_uint operator/(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs /= rhs);
	}
	template <class T>
	friend constexpr big_uint operator/(big_uint lhs, const T& rhs) {
		return std::move(lhs) / big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator/(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) / rhs;
	}
	friend constexpr big_uint operator%(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs %= rhs);
	}
	template <class T>
	friend constexpr big_uint operator%(big_uint lhs, const T& rhs) {
		return std::move(lhs) % big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator%(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) % rhs;
	}
	
	friend constexpr big_uint operator<<(big_uint lhs, std::size_t rhs) {
		return std::move(lhs <<= rhs);
	}
	friend constexpr big_uint operator>>(big_uint lhs, std::size_t rhs) {
		return std::move(lhs >>= rhs);
	}
	friend constexpr big_uint operator&(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs &= rhs);
	}
	template <class T>
	friend constexpr big_uint operator&(big_uint lhs, const T& rhs) {
		return std::move(lhs) & big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator&(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) & rhs;
	}
	friend constexpr big_uint operator|(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs |= rhs);
	}
	template <class T>
	friend constexpr big_uint operator|(big_uint lhs, const T& rhs) {
		return std::move(lhs) | big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator|(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) | rhs;
	}
	friend constexpr big_uint operator^(big_uint lhs, const big_uint& rhs) {
		return std::move(lhs ^= rhs);
	}
	template <class T>
	friend constexpr big_uint operator^(big_uint lhs, const T& rhs) {
		return std::move(lhs) ^ big_uint(rhs);
	}
	template <class T>
	friend constexpr big_uint operator^(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) ^ rhs;
	}
	
	friend constexpr bool operator==(const big_uint& lhs, const big_uint& rhs) {
		return lhs.ubits == rhs.ubits && lhs.lbits == rhs.lbits;
	}
	template <class T>
	friend constexpr bool operator==(const big_uint& lhs, const T& rhs) {
		return lhs == big_uint(rhs);
	}
	template <class T>
	friend constexpr bool operator==(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) == rhs;
	}
	friend constexpr bool operator!=(const big_uint& lhs, const big_uint& rhs) {
		return !(lhs == rhs);
	}
	template <class T>
	friend constexpr bool operator!=(const big_uint& lhs, const T& rhs) {
		return lhs != big_uint(rhs);
	}
	template <class T>
	friend constexpr bool operator!=(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) != rhs;
	}
	friend constexpr bool operator<(const big_uint& lhs, const big_uint& rhs) {
		return lhs.ubits < rhs.ubits || (lhs.ubits == rhs.ubits && lhs.lbits < rhs.lbits);
	}
	template <class T>
	friend constexpr bool operator<(const big_uint& lhs, const T& rhs) {
		return lhs < big_uint(rhs);
	}
	template <class T>
	friend constexpr bool operator<(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) < rhs;
	}
	friend constexpr bool operator<=(const big_uint& lhs, const big_uint& rhs) {
		return !(rhs < lhs);
	}
	template <class T>
	friend constexpr bool operator<=(const big_uint& lhs, const T& rhs) {
		return lhs <= big_uint(rhs);
	}
	template <class T>
	friend constexpr bool operator<=(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) <= rhs;
	}
	friend constexpr bool operator>(const big_uint& lhs, const big_uint& rhs) {
		return rhs < lhs;
	}
	template <class T>
	friend constexpr bool operator>(const big_uint& lhs, const T& rhs) {
		return lhs > big_uint(rhs);
	}
	template <class T>
	friend constexpr bool operator>(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) > rhs;
	}
	friend constexpr bool operator>=(const big_uint& lhs, const big_uint& rhs) {
		return !(lhs < rhs);
	}
	template <class T>
	friend constexpr bool operator>=(const big_uint& lhs, const T& rhs) {
		return lhs >= big_uint(rhs);
	}
	template <class T>
	friend constexpr bool operator>=(const T& lhs, const big_uint& rhs) {
		return big_uint(lhs) >= rhs;
	}
	
	friend std::istream& operator>>(std::istream& is, big_uint& x) {
		std::string s;
		is >> s;
		x = 0;
		for (char c : s) {
			if (!std::isdigit(c)) {
				is.setstate(std::ios::failbit);
				break;
			}
			x *= 10;
			x += c - '0';
		}
		return is;
	}
	friend std::ostream& operator<<(std::ostream& os, const big_uint& x) {
		std::string s;
		for (auto y = x; y; y /= 10) s += static_cast<char>(y % 10) + '0';
		std::reverse(std::begin(s), std::end(s));
		if (s.empty()) s = "0";
		os << s;
		return os;
	}
};
