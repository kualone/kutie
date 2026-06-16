#pragma once

#include "kutie/browser_window.hpp"

#include <Windows.h>
#include <optional>

namespace kutie::platform::windows {

constexpr DWORD kDecorationStyleMask =
    WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

constexpr WPARAM kSysCommandDragMove = 0xF012;

DWORD BuildDecorationStyle(const BrowserWindowOptions& config);
DWORD MergeWindowStyle(DWORD current_style, const BrowserWindowOptions& config);

void ApplyFramelessDwmChrome(HWND hwnd, const BrowserWindowOptions& config);
void AdjustMinMaxInfoForWorkArea(MINMAXINFO* minmax, const RECT& work_area, const RECT& monitor_area);

LONG PartialDecorationTopOffset(bool maximized, LONG rect_top, LONG border_y);

RECT ApplyPartialNcCalcRect(RECT rect, POINT borders, LONG top_offset);

std::optional<LRESULT> HandleFramelessNcCalcSize(
    HWND hwnd,
    WPARAM wparam,
    LPARAM lparam,
    const BrowserWindowOptions& config,
    bool maximized);

} // namespace kutie::platform::windows
