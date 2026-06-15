#pragma once

#include <cstdint>

namespace kutie {

struct Color {
    std::uint8_t r = 15;
    std::uint8_t g = 12;
    std::uint8_t b = 41;
    std::uint8_t a = 255;
};

enum class TitleBarMode {
    Native,
    Hidden,
    Overlay  // reserved for macOS phase 2
};

} // namespace kutie
