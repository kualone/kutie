#include "win_frameless.hpp"

#include "win_dpi.hpp"

#include <dwmapi.h>

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

namespace kutie::platform::windows {

namespace {

void ApplyFramelessDwmChromeWin11(HWND hwnd, const ShellConfig& config) {
    const COLORREF border = ShellBorderColor(config);
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));

    if (config.shadow) {
        DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        const DWM_WINDOW_CORNER_PREFERENCE corner =
            static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_ROUND);
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
    } else {
        DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        const DWM_WINDOW_CORNER_PREFERENCE corner =
            static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_DEFAULT);
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
    }
    const MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void ApplyFramelessDwmChromeWin10(HWND hwnd) {
    // Win10 draws a visible grey sizing border when NC rendering stays enabled on
    // frameless WS_THICKFRAME windows. Disable it and keep margins at zero.
    DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    const MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

} // namespace

DWORD BuildDecorationStyleForPlatform(const ShellConfig& config, bool windows11) {
    if (config.decorations) {
        DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (config.resizable) {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }
        return style;
    }
    DWORD style = WS_MINIMIZEBOX;
    if (config.resizable) {
        style |= WS_MAXIMIZEBOX;
        // Frameless resize uses WM_NCHITTEST on the host gutter; WS_THICKFRAME draws
        // a visible grey sizing band on both Win10 and Win11 when combined with DWM.
        (void)windows11;
    }
    return style;
}

DWORD BuildDecorationStyle(const ShellConfig& config) {
    return BuildDecorationStyleForPlatform(config, IsWindows11OrGreater());
}

DWORD MergeWindowStyle(DWORD current_style, const ShellConfig& config) {
    DWORD style = current_style;
    style &= ~kDecorationStyleMask;
    style |= BuildDecorationStyle(config);
    return style;
}

int FramelessResizeBorderPx(UINT dpi) {
    return MulDiv(8, dpi, 96);
}

bool UsesFramelessResizeBorder(const ShellConfig& config, bool maximized) {
    return !config.decorations && config.resizable && !maximized;
}

RECT WebViewBoundsForShell(const RECT& client, const ShellConfig& config, bool maximized, UINT dpi) {
    if (!UsesFramelessResizeBorder(config, maximized)) {
        return client;
    }
    const int border = FramelessResizeBorderPx(dpi);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (width <= border * 2 || height <= border * 2) {
        return client;
    }
    RECT inset = client;
    inset.left += border;
    inset.top += border;
    inset.right -= border;
    inset.bottom -= border;
    return inset;
}

COLORREF ShellBorderColor(const ShellConfig& config) {
    return RGB(config.background.r, config.background.g, config.background.b);
}

void ApplyFramelessDwmChrome(HWND hwnd, const ShellConfig& config) {
    if (!hwnd) {
        return;
    }

    if (config.decorations) {
        DWMNCRENDERINGPOLICY policy = DWMNCRP_USEWINDOWSTYLE;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        if (IsWindows11OrGreater()) {
            const DWORD border_default = 0xFFFFFFFF;
            DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border_default, sizeof(border_default));
            const DWM_WINDOW_CORNER_PREFERENCE corner =
                static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_DEFAULT);
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
        }
        const MARGINS margins = {0, 0, 0, 0};
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        return;
    }

    if (IsWindows11OrGreater()) {
        ApplyFramelessDwmChromeWin11(hwnd, config);
    } else {
        ApplyFramelessDwmChromeWin10(hwnd);
    }
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

LRESULT HitTestFramelessClient(
    const POINT& client_pt,
    const RECT& client_rect,
    int border_px,
    bool resizable,
    bool maximized) {
    if (!resizable || maximized || border_px <= 0) {
        return HTCLIENT;
    }
    const int width = client_rect.right - client_rect.left;
    const int height = client_rect.bottom - client_rect.top;
    const bool top = client_pt.y < border_px;
    const bool bottom = client_pt.y >= height - border_px;
    const bool left = client_pt.x < border_px;
    const bool right = client_pt.x >= width - border_px;
    if (top && left) {
        return HTTOPLEFT;
    }
    if (top && right) {
        return HTTOPRIGHT;
    }
    if (bottom && left) {
        return HTBOTTOMLEFT;
    }
    if (bottom && right) {
        return HTBOTTOMRIGHT;
    }
    if (top) {
        return HTTOP;
    }
    if (bottom) {
        return HTBOTTOM;
    }
    if (left) {
        return HTLEFT;
    }
    if (right) {
        return HTRIGHT;
    }
    return HTCLIENT;
}

LRESULT HitTestFrameless(HWND hwnd, LPARAM lparam, bool resizable, bool maximized) {
    if (!resizable || maximized) {
        return HTCLIENT;
    }
    POINT pt{
        static_cast<LONG>(static_cast<short>(LOWORD(lparam))),
        static_cast<LONG>(static_cast<short>(HIWORD(lparam)))};
    ScreenToClient(hwnd, &pt);
    RECT client{};
    GetClientRect(hwnd, &client);
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    UINT dpi = GetSystemDpi();
    if (const auto get_dpi = reinterpret_cast<GetDpiForWindowFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"))) {
        dpi = get_dpi(hwnd);
    }
    const int border = FramelessResizeBorderPx(dpi);
    return HitTestFramelessClient(pt, client, border, resizable, maximized);
}

} // namespace kutie::platform::windows
