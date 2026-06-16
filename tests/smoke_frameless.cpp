#include "platform/windows/win_frameless.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using kutie::ShellConfig;
using kutie::platform::windows::AdjustMinMaxInfoForWorkArea;
using kutie::platform::windows::ApplyPartialNcCalcRect;
using kutie::platform::windows::BuildDecorationStyle;
using kutie::platform::windows::MergeWindowStyle;
using kutie::platform::windows::PartialDecorationTopOffset;
using kutie::platform::windows::ShellBorderColor;

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

void TestBuildDecorationStyle() {
    ShellConfig config{};
    config.resizable = true;

    config.decorations = true;
    const DWORD native_style = BuildDecorationStyle(config);
    Expect((native_style & WS_CAPTION) != 0, "native mode keeps WS_CAPTION");
    Expect((native_style & WS_THICKFRAME) != 0, "native resizable keeps WS_THICKFRAME");

    config.decorations = false;
    const DWORD partial_style = BuildDecorationStyle(config);
    Expect((partial_style & WS_CAPTION) != 0, "partial decoration keeps WS_CAPTION like Saucer");
    Expect((partial_style & WS_THICKFRAME) != 0, "partial resizable keeps WS_THICKFRAME");
    Expect((partial_style & WS_MAXIMIZEBOX) != 0, "partial resizable keeps WS_MAXIMIZEBOX");

    config.resizable = false;
    const DWORD locked_style = BuildDecorationStyle(config);
    Expect((locked_style & WS_THICKFRAME) == 0, "non-resizable removes WS_THICKFRAME");
}

void TestMergeWindowStylePreservesVisible() {
    ShellConfig partial{};
    partial.decorations = false;
    partial.resizable = true;
    const DWORD merged = MergeWindowStyle(WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, partial);
    Expect((merged & WS_VISIBLE) != 0, "MergeWindowStyle preserves WS_VISIBLE");
    Expect((merged & WS_CAPTION) != 0, "MergeWindowStyle keeps WS_CAPTION for partial");
    Expect((merged & WS_THICKFRAME) != 0, "MergeWindowStyle keeps WS_THICKFRAME for partial resizable");
}

void TestPartialDecorationTopOffset() {
    Expect(PartialDecorationTopOffset(true, false, 0, 8) == 1, "Win11 windowed uses 1px top offset");
    Expect(PartialDecorationTopOffset(false, false, 0, 8) == 0, "Win10 windowed keeps 0px top offset");
    Expect(PartialDecorationTopOffset(true, true, -1, 8) == 8, "maximized negative top uses border height");
    Expect(PartialDecorationTopOffset(true, true, 0, 8) == 1, "maximized non-negative top keeps Win11 offset");
}

void TestApplyPartialNcCalcRect() {
    const RECT input{0, 0, 800, 600};
    const POINT borders{8, 8};

    const RECT win11 = ApplyPartialNcCalcRect(input, borders, 1);
    Expect(win11.top == 1, "partial client applies top offset");
    Expect(win11.left == 8 && win11.right == 792, "partial client insets left and right");
    Expect(win11.bottom == 592, "partial client insets bottom");

    const RECT win10 = ApplyPartialNcCalcRect(input, borders, 0);
    Expect(win10.top == 0, "Win10 partial keeps client top at origin");
}

void TestShellBorderColor() {
    ShellConfig config{};
    config.background = kutie::Color{18, 16, 32, 255};
    const COLORREF border = ShellBorderColor(config);
    Expect(GetRValue(border) == 18, "border color R matches shell background");
    Expect(GetGValue(border) == 16, "border color G matches shell background");
    Expect(GetBValue(border) == 32, "border color B matches shell background");
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
    TestPartialDecorationTopOffset();
    TestApplyPartialNcCalcRect();
    TestShellBorderColor();
    TestAdjustMinMaxInfoForWorkArea();

    if (g_failures == 0) {
        std::cout << "smoke_frameless: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "smoke_frameless: " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}
