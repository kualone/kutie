#include "kutie/runtime.hpp"

#include "kutie/platform/shell_factory.hpp"
#include "kutie/platform_services.hpp"

#include <Windows.h>

#include <nlohmann/json.hpp>

#include <thread>

namespace kutie {

Runtime::Runtime(const Config& config)
    : config_(config) {
    LoadAssets();
    shell_ = CreateShell(config_.shell, assets(), ipc());
    RegisterBuiltInHandlers();
}

Runtime::~Runtime() = default;

void Runtime::OnReady(ReadyHandler handler) {
    ready_handler_ = std::move(handler);
}

int Runtime::Run() {
    if (ready_handler_) {
        ready_handler_(*this);
    }
    return shell_->Run();
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
    ipc().RegisterHandler("shell.minimize", [this](const nlohmann::json&) {
        shell().Minimize();
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.maximize", [this](const nlohmann::json&) {
        shell().Maximize();
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.restore", [this](const nlohmann::json&) {
        shell().Restore();
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.toggle_maximize", [this](const nlohmann::json&) {
        shell().ToggleMaximize();
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.close", [this](const nlohmann::json&) {
        shell().Close();
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.start_drag", [this](const nlohmann::json&) {
        shell().StartDrag();
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.set_title", [this](const nlohmann::json& payload) {
        shell().SetTitle(payload.value("title", ""));
        return nlohmann::json{{"ok", true}};
    });
    ipc().RegisterHandler("shell.set_decorations", [this](const nlohmann::json& payload) {
        shell().SetDecorations(payload.value("decorations", true));
        return nlohmann::json{{"ok", true}};
    });
}

} // namespace kutie
