#include "platform/windows/win_frameless.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using kutie::ShellConfig;
using kutie::platform::windows::BuildDecorationStyle;
using kutie::platform::windows::HitTestFramelessClient;
using kutie::platform::windows::MergeWindowStyle;

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
    Expect((frameless_style & WS_CAPTION) == 0, "frameless removes WS_CAPTION");
    Expect((frameless_style & WS_THICKFRAME) != 0, "frameless resizable keeps WS_THICKFRAME");

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

} // namespace

int main() {
    TestBuildDecorationStyle();
    TestMergeWindowStylePreservesVisible();
    TestHitTestFramelessClient();

    if (g_failures == 0) {
        std::cout << "smoke_frameless: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "smoke_frameless: " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}
