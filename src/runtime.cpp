#include "kutie/runtime.hpp"

#include "kutie/platform_services.hpp"

#include "platform/windows/win_dpi.hpp"
#include "platform/windows/win_path_util.hpp"
#include "platform/windows/win_string_util.hpp"
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

using WindowVoidMethod = void (BrowserWindow::*)();

void RegisterWindowVoidHandler(IpcHub& ipc, const char* name, WindowVoidMethod method) {
    ipc.RegisterHandler(name, [method](const nlohmann::json& payload) -> nlohmann::json {
        (ResolveWindow(payload)->*method)();
        return nlohmann::json{{"ok", true}};
    });
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

    const std::vector<std::wstring> candidates =
        platform::windows::BuildFrontendCandidates(config_.dev_frontend_path);
    for (const auto& candidate : candidates) {
        if (const auto folder = platform::windows::NormalizeExistingDirectory(candidate)) {
            assets().LoadFromDisk(platform::windows::WideToUtf8(*folder));
            if (!assets().Empty()) {
                break;
            }
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

    RegisterWindowVoidHandler(ipc, "window.show", &BrowserWindow::Show);
    RegisterWindowVoidHandler(ipc, "window.hide", &BrowserWindow::Hide);
    RegisterWindowVoidHandler(ipc, "window.focus", &BrowserWindow::Focus);
    RegisterWindowVoidHandler(ipc, "window.minimize", &BrowserWindow::Minimize);
    RegisterWindowVoidHandler(ipc, "window.maximize", &BrowserWindow::Maximize);
    RegisterWindowVoidHandler(ipc, "window.restore", &BrowserWindow::Restore);
    RegisterWindowVoidHandler(ipc, "window.toggleMaximize", &BrowserWindow::ToggleMaximize);
    RegisterWindowVoidHandler(ipc, "window.startDrag", &BrowserWindow::StartDrag);

    ipc.RegisterHandler("window.setTitle", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->SetTitle(payload.value("title", ""));
        return nlohmann::json{{"ok", true}};
    });

    ipc.RegisterHandler("window.setFrame", [](const nlohmann::json& payload) -> nlohmann::json {
        ResolveWindow(payload)->SetFrame(payload.value("frame", true));
        return nlohmann::json{{"ok", true}};
    });
}

} // namespace kutie
