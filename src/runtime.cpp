#include "kutie/runtime.hpp"

#include "kutie/platform_services.hpp"

#include "platform/windows/win_dpi.hpp"
#include "platform/windows/win_webview_host.hpp"

#include <Windows.h>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <thread>

namespace kutie {

namespace {

BrowserWindowOptions ParseWindowOptions(const nlohmann::json& payload) {
    BrowserWindowOptions options;
    options.title = payload.value("title", options.title);
    options.width = payload.value("width", options.width);
    options.height = payload.value("height", options.height);
    options.center = payload.value("center", options.center);
    options.resizable = payload.value("resizable", options.resizable);
    options.always_on_top = payload.value("always_on_top", options.always_on_top);
    options.show = payload.value("show", options.show);
    options.frame = payload.value("frame", options.frame);
    options.shadow = payload.value("shadow", options.shadow);
    options.devtools = payload.value("devtools", options.devtools);
    options.url = payload.value("url", options.url);
    options.parent_id = payload.value("parent_id", 0u);
    options.modal = payload.value("modal", false);
    options.show_in_taskbar = payload.value("show_in_taskbar", options.parent_id == 0);
    return options;
}

BrowserWindow* ResolveWindow(const nlohmann::json& payload) {
    const uint32_t id = payload.value("id", IpcHub::CurrentDispatchSource());
    BrowserWindow* window = BrowserWindow::FromId(id);
    if (!window) {
        throw std::runtime_error("window not found: " + std::to_string(id));
    }
    return window;
}

} // namespace

Runtime::Runtime(const Config& config)
    : config_(config) {
    LoadAssets();
    RegisterBuiltInHandlers();
}

Runtime::~Runtime() = default;

void Runtime::OnReady(ReadyHandler handler) {
    ready_handler_ = std::move(handler);
}

int Runtime::Run() {
    platform::windows::EnsurePerMonitorDpiAwareness();

    const HRESULT com_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool com_initialized = SUCCEEDED(com_hr) || com_hr == RPC_E_CHANGED_MODE;

    try {
        BrowserWindow::Create(config_.main_window);
    } catch (const std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "Kutie", MB_OK | MB_ICONERROR);
        if (com_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    if (ready_handler_) {
        ready_handler_(*this);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        BrowserWindow::FlushPendingDeletes();
    }

    if (com_initialized) {
        CoUninitialize();
    }
    return static_cast<int>(message.wParam);
}

void Runtime::LoadAssets() {
    assets().LoadEmbedded(GetModuleHandleW(nullptr));
    if (!assets().Empty()) {
        return;
    }

    WCHAR exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    WCHAR* slash = wcsrchr(exe_path, L'\\');
    if (slash) {
        *slash = L'\0';
    }

    std::vector<std::wstring> candidates;
    if (!config_.dev_frontend_path.empty()) {
        const int length = MultiByteToWideChar(
            CP_UTF8, 0, config_.dev_frontend_path.c_str(), -1, nullptr, 0);
        candidates.emplace_back(static_cast<size_t>(length - 1), L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0, config_.dev_frontend_path.c_str(), -1, candidates.back().data(), length);
    }
    candidates.push_back(std::wstring(exe_path) + L"\\frontend");

    for (const auto& candidate : candidates) {
        WCHAR absolute[MAX_PATH];
        if (!GetFullPathNameW(candidate.c_str(), MAX_PATH, absolute, nullptr)) {
            continue;
        }
        if (GetFileAttributesW(absolute) == INVALID_FILE_ATTRIBUTES) {
            continue;
        }

        const int length = WideCharToMultiByte(CP_UTF8, 0, absolute, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8_path(static_cast<size_t>(length - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, absolute, -1, utf8_path.data(), length, nullptr, nullptr);
        assets().LoadFromDisk(utf8_path);
        if (!assets().Empty()) {
            break;
        }
    }
}

void Runtime::RegisterBuiltInHandlers() {
    auto& ipc = this->ipc();

    ipc.RegisterHandler("window.create", [](const nlohmann::json& payload) -> nlohmann::json {
        const BrowserWindowOptions options = ParseWindowOptions(payload);
        if (const std::string error = ValidateBrowserWindowOptions(options); !error.empty()) {
            throw std::runtime_error(error);
        }
        BrowserWindow& window = BrowserWindow::Create(options);
        return nlohmann::json{{"id", window.Id()}};
    });

    ipc.RegisterHandler("window.close", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Close();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.getAll", [](const nlohmann::json&) -> nlohmann::json {
        nlohmann::json ids = nlohmann::json::array();
        for (BrowserWindow* window : BrowserWindow::GetAll()) {
            ids.push_back(window->Id());
        }
        return nlohmann::json{{"ids", std::move(ids)}};
    });

    ipc.RegisterHandler("window.getFocused", [](const nlohmann::json&) -> nlohmann::json {
        BrowserWindow* window = BrowserWindow::GetFocused();
        return nlohmann::json{{"id", window ? window->Id() : 0}};
    });

    ipc.RegisterHandler("window.show", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Show();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.hide", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Hide();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.focus", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Focus();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.minimize", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Minimize();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.maximize", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Maximize();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.restore", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->Restore();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.toggleMaximize", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->ToggleMaximize();
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.setTitle", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->SetTitle(payload.value("title", ""));
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.setFrame", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->SetFrame(payload.value("frame", true));
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.startDrag", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->StartDrag();
        return nlohmann::json{{"ok", true}};
    });
}

} // namespace kutie
