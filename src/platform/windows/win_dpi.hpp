#pragma once

#include <Windows.h>

namespace kutie::platform::windows {

void EnsurePerMonitorDpiAwareness();
UINT GetSystemDpi();

} // namespace kutie::platform::windows
