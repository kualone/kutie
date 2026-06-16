#pragma once

#include "kutie/shell.hpp"

#include <Windows.h>
#include <optional>

namespace kutie::platform::windows {

constexpr DWORD kDecorationStyleMask =
    WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

constexpr WPARAM kSysCommandDragMove = 0xF012;

DWORD BuildDecorationStyle(const ShellConfig& config);
DWORD MergeWindowStyle(DWORD current_style, const ShellConfig& config);

COLORREF ShellBorderColor(const ShellConfig& config);
void ApplyFramelessDwmChrome(HWND hwnd, const ShellConfig& config);
void AdjustMinMaxInfoForWorkArea(MINMAXINFO* minmax, const RECT& work_area, const RECT& monitor_area);

LONG PartialDecorationTopOffset(bool maximized, LONG rect_top, LONG border_y);

// Saucer partial-decoration client rect: side/bottom border insets; top offset only when maximized.
RECT ApplyPartialNcCalcRect(RECT rect, POINT borders, LONG top_offset);

// std::nullopt => DefWindowProc; 0 => handled.
std::optional<LRESULT> HandleFramelessNcCalcSize(
    HWND hwnd,
    WPARAM wparam,
    LPARAM lparam,
    const ShellConfig& config,
    bool maximized);

} // namespace kutie::platform::windows
