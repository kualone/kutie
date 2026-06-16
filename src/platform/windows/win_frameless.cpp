#include "win_frameless.hpp"

#include <dwmapi.h>

namespace kutie::platform::windows {

DWORD BuildDecorationStyle(const ShellConfig& config) {
    DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    if (config.resizable) {
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }
    return style;
}

DWORD MergeWindowStyle(DWORD current_style, const ShellConfig& config) {
    DWORD style = current_style;
    style &= ~kDecorationStyleMask;
    style |= BuildDecorationStyle(config);
    return style;
}

void ApplyFramelessDwmChrome(HWND hwnd, const ShellConfig& config) {
    if (!hwnd) {
        return;
    }

    if (config.decorations) {
        const MARGINS margins{};
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        return;
    }

    // Partial decoration: extend top frame so DWM blends with the custom titlebar.
    const MARGINS margins{0, 0, 2, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

LONG PartialDecorationTopOffset(bool maximized, LONG rect_top, LONG border_y) {
    if (!maximized || rect_top >= 0) {
        return 0L;
    }
    return border_y;
}

RECT ApplyPartialNcCalcRect(RECT rect, POINT borders, LONG top_offset) {
    rect.top += top_offset;
    rect.bottom -= borders.y;
    rect.left += borders.x;
    rect.right -= borders.x;
    return rect;
}

std::optional<LRESULT> HandleFramelessNcCalcSize(
    HWND hwnd,
    WPARAM wparam,
    LPARAM lparam,
    const ShellConfig& config,
    bool maximized) {
    if (config.decorations || wparam == FALSE) {
        return std::nullopt;
    }

    auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
    RECT* rect = &params->rgrc[0];

    WINDOWINFO info{};
    info.cbSize = sizeof(info);
    if (!GetWindowInfo(hwnd, &info)) {
        return 0;
    }

    const POINT borders{
        static_cast<LONG>(info.cxWindowBorders),
        static_cast<LONG>(info.cyWindowBorders),
    };
    const LONG top_offset = PartialDecorationTopOffset(maximized, rect->top, borders.y);
    *rect = ApplyPartialNcCalcRect(*rect, borders, top_offset);
    return 0;
}

void AdjustMinMaxInfoForWorkArea(MINMAXINFO* minmax, const RECT& work_area, const RECT& monitor_area) {
    if (!minmax) {
        return;
    }
    minmax->ptMaxPosition.x = work_area.left - monitor_area.left;
    minmax->ptMaxPosition.y = work_area.top - monitor_area.top;
    minmax->ptMaxSize.x = work_area.right - work_area.left;
    minmax->ptMaxSize.y = work_area.bottom - work_area.top;
}

} // namespace kutie::platform::windows
