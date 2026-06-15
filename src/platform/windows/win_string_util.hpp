#pragma once

#include <string>

namespace kutie::platform::windows {

std::wstring Utf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);

} // namespace kutie::platform::windows
