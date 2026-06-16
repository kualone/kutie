#include "platform/windows/win_frameless.hpp"

#include "kutie/browser_window.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using kutie::BrowserWindowOptions;
using kutie::platform::windows::AdjustMinMaxInfoForWorkArea;
using kutie::platform::windows::ApplyPartialNcCalcRect;
using kutie::platform::windows::BuildBaseWindowStyle;
using kutie::platform::windows::BuildDecorationStyle;
using kutie::platform::windows::MergeWindowStyle;
using kutie::platform::windows::PartialDecorationTopOffset;

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

void TestBuildBaseWindowStyle() {
    BrowserWindowOptions native{};
    native.frame = true;
    native.resizable = true;
    const DWORD native_style = BuildBaseWindowStyle(native);
    Expect(native_style == WS_OVERLAPPEDWINDOW, "native resizable frame uses WS_OVERLAPPEDWINDOW");

    BrowserWindowOptions partial{};
    partial.frame = false;
    partial.resizable = true;
    const DWORD partial_style = BuildBaseWindowStyle(partial);
    Expect((partial_style & WS_CAPTION) != 0, "partial frame keeps WS_CAPTION");
    Expect((partial_style & WS_MAXIMIZEBOX) != 0, "partial resizable keeps WS_MAXIMIZEBOX");
}

void TestBuildDecorationStyle() {
    BrowserWindowOptions config{};
    config.resizable = true;

    config.frame = true;
    const DWORD native_style = BuildBaseWindowStyle(config);
    Expect((native_style & WS_CAPTION) != 0, "native mode keeps WS_CAPTION");
    Expect((native_style & WS_THICKFRAME) != 0, "native resizable keeps WS_THICKFRAME");

    config.frame = false;
    const DWORD partial_style = BuildDecorationStyle(config);
    Expect((partial_style & WS_CAPTION) != 0, "partial decoration keeps WS_CAPTION like Saucer");
    Expect((partial_style & WS_THICKFRAME) != 0, "partial resizable keeps WS_THICKFRAME");
    Expect((partial_style & WS_MAXIMIZEBOX) != 0, "partial resizable keeps WS_MAXIMIZEBOX");

    config.resizable = false;
    const DWORD locked_style = BuildDecorationStyle(config);
    Expect((locked_style & WS_THICKFRAME) == 0, "non-resizable removes WS_THICKFRAME");
}

void TestMergeWindowStylePreservesVisible() {
    BrowserWindowOptions partial{};
    partial.frame = false;
    partial.resizable = true;
    const DWORD merged = MergeWindowStyle(WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, partial);
    Expect((merged & WS_VISIBLE) != 0, "MergeWindowStyle preserves WS_VISIBLE");
    Expect((merged & WS_CAPTION) != 0, "MergeWindowStyle keeps WS_CAPTION for partial");
    Expect((merged & WS_THICKFRAME) != 0, "MergeWindowStyle keeps WS_THICKFRAME for partial resizable");
}

void TestPartialDecorationTopOffset() {
    Expect(PartialDecorationTopOffset(false, 0, 8) == 0, "windowed keeps 0px top offset");
    Expect(PartialDecorationTopOffset(true, -1, 8) == 8, "maximized negative top uses border height");
    Expect(PartialDecorationTopOffset(true, 0, 8) == 0, "maximized non-negative top keeps 0px offset");
}

void TestApplyPartialNcCalcRect() {
    const RECT input{0, 0, 800, 600};
    const POINT borders{8, 8};

    const RECT partial = ApplyPartialNcCalcRect(input, borders, 0);
    Expect(partial.top == 0, "partial client keeps top at origin");
    Expect(partial.left == 8 && partial.right == 792, "partial client insets left and right");
    Expect(partial.bottom == 592, "partial client insets bottom");

    const RECT maximized = ApplyPartialNcCalcRect(input, borders, 8);
    Expect(maximized.top == 8, "maximized negative top applies border height offset");
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
    TestBuildBaseWindowStyle();
    TestBuildDecorationStyle();
    TestMergeWindowStylePreservesVisible();
    TestPartialDecorationTopOffset();
    TestApplyPartialNcCalcRect();
    TestAdjustMinMaxInfoForWorkArea();

    if (g_failures == 0) {
        std::cout << "smoke_frameless: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "smoke_frameless: " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}
