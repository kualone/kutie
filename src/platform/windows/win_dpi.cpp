#include "win_dpi.hpp"
#include "win_string_util.hpp"

#include <shellscalingapi.h>

namespace kutie::platform::windows {

void EnsurePerMonitorDpiAwareness() {
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    if (const auto set_context = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext"))) {
        if (set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            return;
        }
    }

    using SetProcessDpiAwarenessFn = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
    if (const auto set_awareness = reinterpret_cast<SetProcessDpiAwarenessFn>(
            GetProcAddress(GetModuleHandleW(L"shcore.dll"), "SetProcessDpiAwareness"))) {
        set_awareness(PROCESS_PER_MONITOR_DPI_AWARE);
    }
}

UINT GetSystemDpi() {
    using GetDpiForSystemFn = UINT(WINAPI*)();
    if (const auto get_dpi = reinterpret_cast<GetDpiForSystemFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForSystem"))) {
        return get_dpi();
    }
    return 96;
}

COLORREF ToColorRef(const kutie::Color& color) {
    return RGB(color.r, color.g, color.b);
}

kutie::Color FromColorRef(COLORREF color) {
    return kutie::Color{
        GetRValue(color),
        GetGValue(color),
        GetBValue(color),
        255
    };
}

} // namespace kutie::platform::windows
