#include "platform/windows/win_frameless.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using kutie::ShellConfig;
using kutie::platform::windows::AdjustMinMaxInfoForWorkArea;
using kutie::platform::windows::BuildDecorationStyle;
using kutie::platform::windows::BuildDecorationStyleForPlatform;
using kutie::platform::windows::HitTestFramelessClient;
using kutie::platform::windows::MergeWindowStyle;
using kutie::platform::windows::ShellBorderColor;
using kutie::platform::windows::UsesFramelessResizeBorder;
using kutie::platform::windows::WebViewBoundsForShell;

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

void TestBuildDecorationStyle() {
    ShellConfig native{};
    native.decorations = true;
    native.resizable = true;
    const DWORD native_style = BuildDecorationStyle(native);
    Expect((native_style & WS_CAPTION) != 0, "native mode keeps WS_CAPTION");
    Expect((native_style & WS_THICKFRAME) != 0, "native resizable keeps WS_THICKFRAME");

    ShellConfig frameless{};
    frameless.decorations = false;
    frameless.resizable = true;
    const DWORD frameless_win11 = BuildDecorationStyleForPlatform(frameless, true);
    Expect((frameless_win11 & WS_CAPTION) == 0, "frameless removes WS_CAPTION");
    Expect((frameless_win11 & WS_THICKFRAME) == 0, "frameless omits WS_THICKFRAME");

    const DWORD frameless_win10 = BuildDecorationStyleForPlatform(frameless, false);
    Expect((frameless_win10 & WS_THICKFRAME) == 0, "Win10 frameless omits WS_THICKFRAME");
    Expect((frameless_win10 & WS_MAXIMIZEBOX) != 0, "Win10 frameless keeps WS_MAXIMIZEBOX");

    frameless.resizable = false;
    const DWORD locked_style = BuildDecorationStyle(frameless);
    Expect((locked_style & WS_THICKFRAME) == 0, "non-resizable frameless removes WS_THICKFRAME");
}

void TestMergeWindowStylePreservesVisible() {
    ShellConfig frameless{};
    frameless.decorations = false;
    frameless.resizable = true;
    const DWORD merged = MergeWindowStyle(WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, frameless);
    Expect((merged & WS_VISIBLE) != 0, "MergeWindowStyle preserves WS_VISIBLE");
    Expect((merged & WS_CAPTION) == 0, "MergeWindowStyle removes WS_CAPTION for frameless");
}

void TestHitTestFramelessClient() {
    const RECT client{0, 0, 800, 600};
    const int border = 8;

    Expect(
        HitTestFramelessClient(POINT{4, 4}, client, border, true, false) == HTTOPLEFT,
        "top-left corner is HTTOPLEFT");
    Expect(
        HitTestFramelessClient(POINT{796, 596}, client, border, true, false) == HTBOTTOMRIGHT,
        "bottom-right corner is HTBOTTOMRIGHT");
    Expect(
        HitTestFramelessClient(POINT{400, 300}, client, border, true, false) == HTCLIENT,
        "center is HTCLIENT");
    Expect(
        HitTestFramelessClient(POINT{4, 4}, client, border, true, true) == HTCLIENT,
        "maximized window has no resize border");
    Expect(
        HitTestFramelessClient(POINT{4, 4}, client, border, false, false) == HTCLIENT,
        "non-resizable window has no resize border");
}

void TestShellBorderColor() {
    ShellConfig config{};
    config.background = kutie::Color{18, 16, 32, 255};
    const COLORREF border = ShellBorderColor(config);
    Expect(GetRValue(border) == 18, "border color R matches shell background");
    Expect(GetGValue(border) == 16, "border color G matches shell background");
    Expect(GetBValue(border) == 32, "border color B matches shell background");
}

void TestWebViewBoundsForShell() {
    const RECT client{0, 0, 800, 600};
    ShellConfig frameless{};
    frameless.decorations = false;
    frameless.resizable = true;

    const RECT inset = WebViewBoundsForShell(client, frameless, false, 96);
    Expect(inset.left == 8, "frameless WebView is inset from left");
    Expect(inset.top == 8, "frameless WebView is inset from top");
    Expect(inset.right == 792, "frameless WebView is inset from right");
    Expect(inset.bottom == 592, "frameless WebView is inset from bottom");

    const RECT full = WebViewBoundsForShell(client, frameless, true, 96);
    Expect(full.right == 800 && full.bottom == 600, "maximized frameless uses full client");
    Expect(UsesFramelessResizeBorder(frameless, false), "frameless resizable exposes resize gutter");
    Expect(!UsesFramelessResizeBorder(frameless, true), "maximized frameless skips resize gutter");
}

void TestAdjustMinMaxInfoForWorkArea() {
    MINMAXINFO minmax{};
    const RECT monitor{0, 0, 1920, 1080};
    const RECT work{0, 0, 1920, 1040};
    AdjustMinMaxInfoForWorkArea(&minmax, work, monitor);
    Expect(minmax.ptMaxPosition.x == 0 && minmax.ptMaxPosition.y == 0, "max position origin at work area");
    Expect(minmax.ptMaxSize.x == 1920 && minmax.ptMaxSize.y == 1040, "max size matches work area");

    const RECT work_offset{100, 50, 1820, 1030};
    const RECT monitor_offset{100, 50, 1920, 1080};
    MINMAXINFO offset{};
    AdjustMinMaxInfoForWorkArea(&offset, work_offset, monitor_offset);
    Expect(offset.ptMaxPosition.x == 0 && offset.ptMaxPosition.y == 0, "max position relative to monitor");
    Expect(offset.ptMaxSize.x == 1720 && offset.ptMaxSize.y == 980, "max size on secondary monitor work area");
}

} // namespace

int main() {
    TestBuildDecorationStyle();
    TestMergeWindowStylePreservesVisible();
    TestHitTestFramelessClient();
    TestShellBorderColor();
    TestWebViewBoundsForShell();
    TestAdjustMinMaxInfoForWorkArea();

    if (g_failures == 0) {
        std::cout << "smoke_frameless: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "smoke_frameless: " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}
