#include "kutie/browser_window.hpp"

#include "platform/windows/win_browser_window.hpp"

#include "kutie/asset_bundle.hpp"
#include "kutie/ipc_hub.hpp"

#if defined(KUTIE_PLATFORM_WINDOWS)
#include <Windows.h>
#endif

#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace kutie {

namespace {

std::mutex g_registry_mutex;
std::unordered_map<uint32_t, std::unique_ptr<BrowserWindow>> g_windows;
std::vector<std::unique_ptr<BrowserWindow>> g_pending_deletes;
uint32_t g_next_id = 0;

} // namespace

void BrowserWindow::FlushPendingDeletes() {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    g_pending_deletes.clear();
}

BrowserWindow::BrowserWindow(uint32_t id, BrowserWindowOptions options)
    : id_(id)
    , options_(std::move(options)) {}

BrowserWindow::~BrowserWindow() = default;

uint32_t BrowserWindow::AllocateId() {
    return ++g_next_id;
}

void BrowserWindow::RegisterWindow(uint32_t id, std::unique_ptr<BrowserWindow> window) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    g_windows[id] = std::move(window);
}

void BrowserWindow::NotifyDestroyed() {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    const auto it = g_windows.find(id_);
    if (it == g_windows.end()) {
        return;
    }
    g_pending_deletes.push_back(std::move(it->second));
    g_windows.erase(it);
}

std::optional<uint32_t> BrowserWindow::ParentId() const {
    if (options_.parent_id == 0) {
        return std::nullopt;
    }
    return options_.parent_id;
}

std::string ValidateBrowserWindowOptions(const BrowserWindowOptions& options) {
    if (options.modal && options.parent_id == 0) {
        return "modal requires parent_id";
    }
    if (options.parent_id != 0 && !BrowserWindow::FromId(options.parent_id)) {
        return "parent_id does not exist";
    }
    if (options.width <= 0 || options.height <= 0) {
        return "width and height must be positive";
    }
    return {};
}

BrowserWindow& BrowserWindow::Create(const BrowserWindowOptions& options) {
    if (const std::string error = ValidateBrowserWindowOptions(options); !error.empty()) {
        throw std::invalid_argument(error);
    }

    const uint32_t id = AllocateId();
    auto window = std::make_unique<WinBrowserWindow>(
        id,
        options,
        AssetBundle::Shared(),
        IpcHub::Shared());
    auto* raw = window.get();
    RegisterWindow(id, std::move(window));
    static_cast<WinBrowserWindow*>(raw)->Initialize();
    return *raw;
}

BrowserWindow* BrowserWindow::FromId(uint32_t id) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    const auto it = g_windows.find(id);
    return it == g_windows.end() ? nullptr : it->second.get();
}

BrowserWindow* BrowserWindow::GetFocused() {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    for (const auto& entry : g_windows) {
        if (entry.second->NativeHandle()) {
#if defined(KUTIE_PLATFORM_WINDOWS)
            if (GetForegroundWindow() == static_cast<HWND>(entry.second->NativeHandle())) {
                return entry.second.get();
            }
#endif
        }
    }
    return nullptr;
}

std::vector<BrowserWindow*> BrowserWindow::GetAll() {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    std::vector<BrowserWindow*> windows;
    windows.reserve(g_windows.size());
    for (const auto& entry : g_windows) {
        windows.push_back(entry.second.get());
    }
    return windows;
}

void BrowserWindow::Close() {}
void BrowserWindow::Show() {}
void BrowserWindow::Hide() {}
void BrowserWindow::Focus() {}
void BrowserWindow::SetTitle(const std::string&) {}
void BrowserWindow::SetSize(int, int) {}
void BrowserWindow::SetPosition(int, int) {}
void BrowserWindow::SetResizable(bool) {}
void BrowserWindow::SetFrame(bool) {}
void BrowserWindow::Minimize() {}
void BrowserWindow::Maximize() {}
void BrowserWindow::Restore() {}
void BrowserWindow::ToggleMaximize() {}
void BrowserWindow::StartDrag() {}
void BrowserWindow::ToggleDevTools() {}
void BrowserWindow::ExecuteScript(const std::string&) {}
void BrowserWindow::OnClose(CloseHandler handler) { on_close_ = std::move(handler); }
void BrowserWindow::OnResize(ResizeHandler handler) { on_resize_ = std::move(handler); }
void* BrowserWindow::NativeHandle() const { return nullptr; }

} // namespace kutie