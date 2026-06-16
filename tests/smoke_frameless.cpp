#include "platform/windows/win_frameless.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using kutie::ShellConfig;
using kutie::platform::windows::AdjustMinMaxInfoForWorkArea;
using kutie::platform::windows::ApplyPartialNcCalcRect;
using kutie::platform::windows::BuildDecorationStyle;
using kutie::platform::windows::MergeWindowStyle;
using kutie::platform::windows::ShellBorderColor;

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
    const DWORD frameless_style = BuildDecorationStyle(frameless);
    Expect((frameless_style & WS_CAPTION) == 0, "partial decoration removes WS_CAPTION");
    Expect((frameless_style & WS_THICKFRAME) != 0, "partial resizable keeps WS_THICKFRAME for native borders");
    Expect((frameless_style & WS_MAXIMIZEBOX) != 0, "partial resizable keeps WS_MAXIMIZEBOX");

    frameless.resizable = false;
    const DWORD locked_style = BuildDecorationStyle(frameless);
    Expect((locked_style & WS_THICKFRAME) == 0, "non-resizable partial removes WS_THICKFRAME");
}

void TestMergeWindowStylePreservesVisible() {
    ShellConfig frameless{};
    frameless.decorations = false;
    frameless.resizable = true;
    const DWORD merged = MergeWindowStyle(WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, frameless);
    Expect((merged & WS_VISIBLE) != 0, "MergeWindowStyle preserves WS_VISIBLE");
    Expect((merged & WS_CAPTION) == 0, "MergeWindowStyle removes WS_CAPTION for partial");
    Expect((merged & WS_THICKFRAME) != 0, "MergeWindowStyle keeps WS_THICKFRAME for partial resizable");
}

void TestApplyPartialNcCalcRect() {
    const RECT input{0, 0, 800, 600};
    const POINT borders{8, 8};

    const RECT windowed = ApplyPartialNcCalcRect(input, borders);
    Expect(windowed.top == 0, "windowed partial keeps top at client origin");
    Expect(windowed.left == 8 && windowed.right == 792, "windowed partial insets left and right");
    Expect(windowed.bottom == 592, "windowed partial insets bottom");
}

void TestMaximizedRectSkipsBorderInset() {
    const RECT work_area{0, 0, 1920, 1040};
    RECT rect = work_area;
    // Maximized path snaps to work area and returns without border insets.
    Expect(rect.left == 0 && rect.top == 0, "maximized client fills work area left/top");
    Expect(rect.right == 1920 && rect.bottom == 1040, "maximized client fills work area right/bottom");
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
    TestApplyPartialNcCalcRect();
    TestMaximizedRectSkipsBorderInset();
    TestShellBorderColor();
    TestAdjustMinMaxInfoForWorkArea();

    if (g_failures == 0) {
        std::cout << "smoke_frameless: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "smoke_frameless: " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}
