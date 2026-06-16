#include "kutie/browser_window.hpp"

#include <cstdlib>
#include <iostream>

namespace {

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

void TestValidateModalRequiresParent() {
    kutie::BrowserWindowOptions options;
    options.modal = true;
    options.parent_id = 0;
    Expect(!kutie::ValidateBrowserWindowOptions(options).empty(), "modal without parent_id fails");
}

void TestValidatePositiveDimensions() {
    kutie::BrowserWindowOptions options;
    options.width = 0;
    options.height = 640;
    Expect(!kutie::ValidateBrowserWindowOptions(options).empty(), "non-positive width fails");

    options.width = 800;
    options.height = -1;
    Expect(!kutie::ValidateBrowserWindowOptions(options).empty(), "non-positive height fails");
}

void TestValidateDefaultOptions() {
    kutie::BrowserWindowOptions options;
    Expect(kutie::ValidateBrowserWindowOptions(options).empty(), "default options are valid");
}

} // namespace

int main() {
    TestValidateModalRequiresParent();
    TestValidatePositiveDimensions();
    TestValidateDefaultOptions();

    if (g_failures == 0) {
        std::cout << "smoke_browser_window: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "smoke_browser_window: " << g_failures << " failure(s)\n";
    return EXIT_FAILURE;
}
