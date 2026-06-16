#pragma once

#include <cstdlib>
#include <iostream>

namespace kutie::test {

inline int g_failures = 0;

inline void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

inline int Finish(const char* suite_name) {
    if (g_failures == 0) {
        std::cout << suite_name << ": all tests passed\n";
        return EXIT_SUCCESS;
    }
    std::cerr << suite_name << ": " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}

} // namespace kutie::test
