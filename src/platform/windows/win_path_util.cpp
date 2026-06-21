#include "win_path_util.hpp"

#include "win_string_util.hpp"

#include <ShlObj.h>
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

std::wstring GetLocalAppDataDirectory() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)) || !path) {
        return L"";
    }
    const std::wstring local_app_data(path);
    CoTaskMemFree(path);
    return local_app_data;
}

bool EnsureDirectoryExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    const int result = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
    return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS;
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
