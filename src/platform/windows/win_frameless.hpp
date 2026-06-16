#pragma once

#include "kutie/shell.hpp"

#include <Windows.h>
#include <optional>

namespace kutie::platform::windows {

constexpr DWORD kDecorationStyleMask =
    WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

// Saucer-style partial decoration: custom titlebar + native resize borders + system shadow.
constexpr WPARAM kSysCommandDragMove = 0xF012;

DWORD BuildDecorationStyle(const ShellConfig& config);
DWORD MergeWindowStyle(DWORD current_style, const ShellConfig& config);

COLORREF ShellBorderColor(const ShellConfig& config);
void ExtendFrameIntoClientArea(HWND hwnd, int left, int right, int top, int bottom);
void ApplyFramelessDwmChrome(HWND hwnd, const ShellConfig& config);
void AdjustMinMaxInfoForWorkArea(MINMAXINFO* minmax, const RECT& work_area, const RECT& monitor_area);

// Windowed partial decoration: inset left/right/bottom for native resize borders; no top inset.
RECT ApplyPartialNcCalcRect(RECT rect, POINT borders);

// std::nullopt => DefWindowProc; 0 => handled.
std::optional<LRESULT> HandleFramelessNcCalcSize(
    HWND hwnd,
    WPARAM wparam,
    LPARAM lparam,
    const ShellConfig& config,
    bool maximized);

} // namespace kutie::platform::windows
