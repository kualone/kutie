#include "win_frameless.hpp"

#include "win_dpi.hpp"

namespace kutie::platform::windows {

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
    const int border = MulDiv(8, dpi, 96);
    return HitTestFramelessClient(pt, client, border, resizable, maximized);
}

} // namespace kutie::platform::windows
