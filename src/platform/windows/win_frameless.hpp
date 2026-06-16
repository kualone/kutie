#pragma once

#include "kutie/shell.hpp"

#include <Windows.h>

namespace kutie::platform::windows {

constexpr DWORD kDecorationStyleMask =
    WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

DWORD BuildDecorationStyle(const ShellConfig& config);
DWORD BuildDecorationStyleForPlatform(const ShellConfig& config, bool windows11);
DWORD MergeWindowStyle(DWORD current_style, const ShellConfig& config);

int FramelessResizeBorderPx(UINT dpi);
bool UsesFramelessResizeBorder(const ShellConfig& config, bool maximized);
RECT WebViewBoundsForShell(const RECT& client, const ShellConfig& config, bool maximized, UINT dpi);

COLORREF ShellBorderColor(const ShellConfig& config);
void ApplyFramelessDwmChrome(HWND hwnd, const ShellConfig& config);
void AdjustMinMaxInfoForWorkArea(MINMAXINFO* minmax, const RECT& work_area, const RECT& monitor_area);

// Pure hit-test helper (unit-testable without a live HWND).
LRESULT HitTestFramelessClient(
    const POINT& client_pt,
    const RECT& client_rect,
    int border_px,
    bool resizable,
    bool maximized);

LRESULT HitTestFrameless(HWND hwnd, LPARAM lparam, bool resizable, bool maximized);

} // namespace kutie::platform::windows
