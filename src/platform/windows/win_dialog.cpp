#include "kutie/platform_services.hpp"

#include "win_string_util.hpp"

#include <Windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#pragma comment(lib, "ole32.lib")

namespace {

struct ComApartmentScope {
    ComApartmentScope()
        : initialized_(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {}
    ~ComApartmentScope() {
        if (initialized_) {
            CoUninitialize();
        }
    }

private:
    bool initialized_;
};

void ApplyFilterSpecs(IFileDialog* dialog, const std::vector<kutie::PlatformServices::FileFilter>& filters) {
    std::vector<std::wstring> pattern_storage;
    std::vector<COMDLG_FILTERSPEC> filter_specs;
    for (const auto& filter : filters) {
        pattern_storage.push_back(kutie::platform::windows::Utf8ToWide(filter.label));
        pattern_storage.push_back(kutie::platform::windows::Utf8ToWide(filter.pattern));
        filter_specs.push_back({pattern_storage[pattern_storage.size() - 2].c_str(),
                                pattern_storage[pattern_storage.size() - 1].c_str()});
    }
    if (!filter_specs.empty()) {
        dialog->SetFileTypes(static_cast<UINT>(filter_specs.size()), filter_specs.data());
    }
}

std::optional<std::string> ReadShellItemPath(IShellItem* item) {
    PWSTR path = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
        return std::nullopt;
    }
    const std::string utf8_path = kutie::platform::windows::WideToUtf8(path);
    CoTaskMemFree(path);
    return utf8_path;
}

} // namespace

namespace kutie {

std::vector<std::string> PlatformServices::OpenFiles(void* parent_window, const OpenFileOptions& options) {
    std::vector<std::string> files;
    ComApartmentScope com;

    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return files;
    }

    DWORD flags = 0;
    dialog->GetOptions(&flags);
    flags |= FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST;
    if (options.multi_select) {
        flags |= FOS_ALLOWMULTISELECT;
    }
    dialog->SetOptions(flags);
    dialog->SetTitle(platform::windows::Utf8ToWide(options.title).c_str());
    ApplyFilterSpecs(dialog, options.filters);

    if (dialog->Show(static_cast<HWND>(parent_window)) == S_OK) {
        IShellItemArray* items = nullptr;
        if (SUCCEEDED(dialog->GetResults(&items))) {
            DWORD count = 0;
            items->GetCount(&count);
            for (DWORD index = 0; index < count; ++index) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(items->GetItemAt(index, &item))) {
                    if (const auto path = ReadShellItemPath(item)) {
                        files.push_back(*path);
                    }
                    item->Release();
                }
            }
            items->Release();
        }
    }

    dialog->Release();
    return files;
}

std::optional<std::string> PlatformServices::SaveFile(void* parent_window, const SaveFileOptions& options) {
    ComApartmentScope com;

    IFileSaveDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return std::nullopt;
    }

    DWORD flags = 0;
    dialog->GetOptions(&flags);
    dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT);
    dialog->SetTitle(platform::windows::Utf8ToWide(options.title).c_str());
    ApplyFilterSpecs(dialog, options.filters);
    if (!options.default_name.empty()) {
        dialog->SetFileName(platform::windows::Utf8ToWide(options.default_name).c_str());
    }

    std::optional<std::string> saved;
    if (dialog->Show(static_cast<HWND>(parent_window)) == S_OK) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            saved = ReadShellItemPath(item);
            item->Release();
        }
    }

    dialog->Release();
    return saved;
}

std::optional<std::string> PlatformServices::PickFolder(void* parent_window, const std::string& title) {
    ComApartmentScope com;

    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return std::nullopt;
    }

    DWORD flags = 0;
    dialog->GetOptions(&flags);
    dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
    dialog->SetTitle(platform::windows::Utf8ToWide(title).c_str());

    std::optional<std::string> folder;
    if (dialog->Show(static_cast<HWND>(parent_window)) == S_OK) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            folder = ReadShellItemPath(item);
            item->Release();
        }
    }

    dialog->Release();
    return folder;
}

std::optional<std::string> PlatformServices::ReadClipboardText() {
    if (!OpenClipboard(nullptr)) {
        return std::nullopt;
    }

    std::optional<std::string> text;
    if (const HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto* wide = static_cast<const wchar_t*>(GlobalLock(data))) {
            text = platform::windows::WideToUtf8(wide);
            GlobalUnlock(data);
        }
    }

    CloseClipboard();
    return text;
}

bool PlatformServices::WriteClipboardText(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    EmptyClipboard();
    const std::wstring wide = platform::windows::Utf8ToWide(text);
    const size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!global) {
        CloseClipboard();
        return false;
    }

    void* locked = GlobalLock(global);
    memcpy(locked, wide.c_str(), bytes);
    GlobalUnlock(global);
    SetClipboardData(CF_UNICODETEXT, global);
    CloseClipboard();
    return true;
}

void PlatformServices::ClearClipboard() {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        CloseClipboard();
    }
}

void PlatformServices::ShowInfo(void* parent_window, const std::string& title, const std::string& message) {
    MessageBoxW(
        static_cast<HWND>(parent_window),
        platform::windows::Utf8ToWide(message).c_str(),
        platform::windows::Utf8ToWide(title).c_str(),
        MB_OK | MB_ICONINFORMATION);
}

bool PlatformServices::AskConfirm(void* parent_window, const std::string& title, const std::string& message) {
    const int result = MessageBoxW(
        static_cast<HWND>(parent_window),
        platform::windows::Utf8ToWide(message).c_str(),
        platform::windows::Utf8ToWide(title).c_str(),
        MB_YESNO | MB_ICONQUESTION);
    return result == IDYES;
}

} // namespace kutie
