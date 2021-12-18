#include <iostream>
#include <utility>
#include "big_uint.hpp"


using uint_type = big_uint<512>;

template <std::size_t N>
struct fib {
	static constexpr uint_type value = fib<N - 1>::value + fib<N - 2>::value;
};

template <>
struct fib<0> {
	static constexpr uint_type value = 0;
};
template <>
struct fib<1> {
	static constexpr uint_type value = 1;
};

template <std::size_t N>
constexpr uint_type fib_v = fib<N>::value;


template <std::size_t... I>
void print_fib(std::index_sequence<I...>) {
	((std::cout << fib_v<I> << '\n'), ...);
}

int main() {
	print_fib(std::make_index_sequence<500>{});
}
