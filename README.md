# big_uint

大きな正の整数を扱うためのクラスです。

C++17 以上で動作します。

`big_uint<128>` 、 `big_uint<256>` のように、テンプレート引数にビット数を指定して使います。
ただし、ビット数は 128 以上で 2 の冪である必要があります。

基本的な機能は定数式でも使えます。

## 例

```c++
#include <iostream>
#include "big_uint.hpp"

template <std::size_t Bit>
constexpr big_uint<Bit> fact(big_uint<Bit> n) {
    if (n == 0) return 1;
    return std::move(n) * fact(n - 1);
}

int main() {
    big_uint<128> a = 12345;

    //big_uint<128> b = 123456789012345678901234567890;  // ng
    big_uint<128> b = "123456789012345678901234567890";  // ok

    constexpr auto c = fact(big_uint<1024>(100));

    std::cout << a << std::endl;  // 12345
    std::cout << b << std::endl;  // 123456789012345678901234567890
    std::cout << c << std::endl;  // 93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000
}
```
