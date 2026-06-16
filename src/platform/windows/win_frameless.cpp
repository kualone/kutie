#include "win_frameless.hpp"

#include "win_dpi.hpp"

#include <dwmapi.h>
#include <shellapi.h>

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

namespace kutie::platform::windows {

namespace {

bool TaskbarAutoHideEdge(UINT edge) {
    APPBARDATA data{};
    data.cbSize = sizeof(data);
    data.uEdge = edge;
    return SHAppBarMessage(ABM_GETAUTOHIDEBAR, &data) != 0;
}

void SnapMaximizedRectToWorkArea(RECT* rect) {
    if (!rect) {
        return;
    }
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    const HMONITOR monitor = MonitorFromRect(rect, MONITOR_DEFAULTTONULL);
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }
    *rect = monitor_info.rcWork;
    if (TaskbarAutoHideEdge(ABE_BOTTOM)) {
        rect->bottom -= 1;
    }
    if (TaskbarAutoHideEdge(ABE_LEFT)) {
        rect->left += 1;
    }
    if (TaskbarAutoHideEdge(ABE_TOP)) {
        rect->top += 1;
    }
    if (TaskbarAutoHideEdge(ABE_RIGHT)) {
        rect->right -= 1;
    }
}

} // namespace

DWORD BuildDecorationStyle(const ShellConfig& config) {
    if (config.decorations) {
        DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (config.resizable) {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }
        return style;
    }
    DWORD style = WS_MINIMIZEBOX;
    if (config.resizable) {
        style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
    }
    return style;
}

DWORD MergeWindowStyle(DWORD current_style, const ShellConfig& config) {
    DWORD style = current_style;
    style &= ~kDecorationStyleMask;
    style |= BuildDecorationStyle(config);
    return style;
}

COLORREF ShellBorderColor(const ShellConfig& config) {
    return RGB(config.background.r, config.background.g, config.background.b);
}

void ExtendFrameIntoClientArea(HWND hwnd, int left, int right, int top, int bottom) {
    if (!hwnd) {
        return;
    }
    const MARGINS margins{left, right, top, bottom};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void ApplyFramelessDwmChrome(HWND hwnd, const ShellConfig& config) {
    if (!hwnd) {
        return;
    }

    if (config.decorations) {
        DWMNCRENDERINGPOLICY policy = DWMNCRP_USEWINDOWSTYLE;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        ExtendFrameIntoClientArea(hwnd, 0, 0, 0, 0);
        if (IsWindows11OrGreater()) {
            const DWORD border_default = 0xFFFFFFFF;
            DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border_default, sizeof(border_default));
            const DWM_WINDOW_CORNER_PREFERENCE corner =
                static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_DEFAULT);
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
        }
        return;
    }

    const COLORREF border = ShellBorderColor(config);
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));

    if (config.shadow) {
        DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        if (IsWindows11OrGreater()) {
            const DWM_WINDOW_CORNER_PREFERENCE corner =
                static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_ROUND);
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
        }
    } else {
        DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        if (IsWindows11OrGreater()) {
            const DWM_WINDOW_CORNER_PREFERENCE corner =
                static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_DEFAULT);
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
        }
    }

    // Partial decoration: extend top frame so DWM blends with the custom titlebar.
    ExtendFrameIntoClientArea(hwnd, 0, 0, 2, 0);
}

RECT ApplyPartialNcCalcRect(RECT rect, POINT borders) {
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

    if (maximized) {
        SnapMaximizedRectToWorkArea(rect);
        return 0;
    }

    WINDOWINFO info{};
    info.cbSize = sizeof(info);
    if (!GetWindowInfo(hwnd, &info)) {
        return 0;
    }

    const POINT borders{
        static_cast<LONG>(info.cxWindowBorders),
        static_cast<LONG>(info.cyWindowBorders),
    };
    *rect = ApplyPartialNcCalcRect(*rect, borders);
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
