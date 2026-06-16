#pragma once

#include "kutie/types.hpp"

#include <Windows.h>

namespace kutie::platform::windows {

void EnsurePerMonitorDpiAwareness();
UINT GetSystemDpi();
bool IsWindows11OrGreater();
COLORREF ToColorRef(const Color& color);
Color FromColorRef(COLORREF color);

} // namespace kutie::platform::windows
