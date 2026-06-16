#include "win_path_util.hpp"

#include "win_string_util.hpp"

#include <Windows.h>

namespace kutie::platform::windows {

std::wstring GetExeDirectory() {
    WCHAR exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    WCHAR* slash = wcsrchr(exe_path, L'\\');
    if (slash) {
        *slash = L'\0';
    }
    return exe_path;
}

std::optional<std::wstring> NormalizeExistingDirectory(const std::wstring& candidate) {
    WCHAR absolute[MAX_PATH];
    if (!GetFullPathNameW(candidate.c_str(), MAX_PATH, absolute, nullptr)) {
        return std::nullopt;
    }
    if (GetFileAttributesW(absolute) == INVALID_FILE_ATTRIBUTES) {
        return std::nullopt;
    }
    return absolute;
}

std::optional<std::wstring> ResolveExistingDirectory(const std::vector<std::wstring>& candidates) {
    for (const auto& candidate : candidates) {
        if (const auto folder = NormalizeExistingDirectory(candidate)) {
            return folder;
        }
    }
    return std::nullopt;
}

std::vector<std::wstring> BuildFrontendCandidates(const std::string& dev_frontend_path) {
    std::vector<std::wstring> candidates;
    if (!dev_frontend_path.empty()) {
        candidates.push_back(Utf8ToWide(dev_frontend_path));
    }
    const std::wstring exe_dir = GetExeDirectory();
    candidates.push_back(exe_dir + L"\\..\\..\\sample\\frontend");
    candidates.push_back(exe_dir + L"\\frontend");
    return candidates;
}

} // namespace kutie::platform::windows
