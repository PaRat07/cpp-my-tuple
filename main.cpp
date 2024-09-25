#include "my-tuple.h"

#include <iostream>

int main() {
    Tuple<size_t, int, size_t, int> t;
    auto [a1, a2, a3, a4] = t;
    std::tuple<size_t, int, size_t, int> tstd;
    std::cout << sizeof(t) << " " << sizeof(tstd) << std::endl;
    return 0;
}
