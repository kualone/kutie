#pragma once

#include <optional>
#include <string>
#include <vector>

namespace kutie::platform::windows {

std::wstring GetExeDirectory();
std::wstring GetLocalAppDataDirectory();
bool EnsureDirectoryExists(const std::wstring& path);
std::optional<std::wstring> NormalizeExistingDirectory(const std::wstring& candidate);
std::optional<std::wstring> ResolveExistingDirectory(const std::vector<std::wstring>& candidates);
std::vector<std::wstring> BuildFrontendCandidates(const std::string& dev_frontend_path);

} // namespace kutie::platform::windows
