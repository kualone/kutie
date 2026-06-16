#include "kutie/browser_window.hpp"
#include "test_util.hpp"

namespace {

using kutie::test::Expect;
using kutie::test::Finish;

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
    return Finish("smoke_browser_window");
}
