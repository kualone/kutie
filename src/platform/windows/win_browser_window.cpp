#include "win_browser_window.hpp"

#include "win_dpi.hpp"
#include "win_frameless.hpp"
#include "win_path_util.hpp"
#include "win_string_util.hpp"
#include "win_webview_host.hpp"

#include <dwmapi.h>
#include <nlohmann/json.hpp>
#include <shellscalingapi.h>
#include <wrl/client.h>
#include <wrl/event.h>

#pragma comment(lib, "dwmapi.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace kutie {

namespace {

constexpr wchar_t kWindowClassName[] = L"KutieBrowserWindow";
constexpr wchar_t kVirtualHost[] = L"assets.kutie";
constexpr char kVirtualOrigin[] = "https://assets.kutie";

constexpr UINT WM_KUTIE_APPLY_STYLE = WM_USER + 1;
constexpr UINT WM_KUTIE_START_DRAG = WM_USER + 2;
constexpr UINT WM_KUTIE_MINIMIZE = WM_USER + 3;
constexpr UINT WM_KUTIE_MAXIMIZE = WM_USER + 4;
constexpr UINT WM_KUTIE_RESTORE = WM_USER + 5;
constexpr UINT WM_KUTIE_TOGGLE_MAXIMIZE = WM_USER + 6;
constexpr UINT WM_KUTIE_EXECUTE_SCRIPT = WM_USER + 7;

std::wstring MakeFallbackFolder(AssetBundle& assets) {
    WCHAR temp_dir[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_dir);
    const std::wstring folder = std::wstring(temp_dir) + L"kutie_assets_" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(folder.c_str(), nullptr);

    for (const std::string& path : assets.Paths()) {
        AssetBundle::Asset asset;
        if (!assets.Resolve(path, asset)) {
            continue;
        }

        std::wstring relative = platform::windows::Utf8ToWide(path);
        if (!relative.empty() && relative.front() == L'/') {
            relative.erase(relative.begin());
        }
        for (auto& ch : relative) {
            if (ch == L'/') {
                ch = L'\\';
            }
        }

        const std::wstring full_path = folder + L"\\" + relative;
        const size_t slash = full_path.find_last_of(L'\\');
        if (slash != std::wstring::npos) {
            std::wstring parent = full_path.substr(0, slash);
            for (size_t pos = folder.size(); pos < parent.size();) {
                pos = parent.find(L'\\', pos);
                if (pos == std::wstring::npos) {
                    pos = parent.size();
                }
                CreateDirectoryW(parent.substr(0, pos).c_str(), nullptr);
                ++pos;
            }
        }

        HANDLE file = CreateFileW(
            full_path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            continue;
        }

        DWORD written = 0;
        WriteFile(file, asset.bytes.data(), static_cast<DWORD>(asset.bytes.size()), &written, nullptr);
        CloseHandle(file);
    }

    return folder;
}

std::wstring ResolveFrontendFolder() {
    const std::vector<std::wstring> candidates =
        platform::windows::BuildFrontendCandidates({});
    return platform::windows::ResolveExistingDirectory(candidates).value_or(L"");
}

HWND ResolveOwnerHwnd(uint32_t parent_id) {
    if (parent_id == 0) {
        return nullptr;
    }
    BrowserWindow* parent = BrowserWindow::FromId(parent_id);
    if (!parent) {
        return nullptr;
    }
    return static_cast<HWND>(parent->NativeHandle());
}

} // namespace

void WinBrowserWindow::RegisterWindowClass() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    window_class.lpszClassName = kWindowClassName;
    RegisterClassExW(&window_class);
}

WinBrowserWindow::WinBrowserWindow(
    uint32_t id,
    BrowserWindowOptions options,
    AssetBundle& assets,
    IpcHub& ipc)
    : BrowserWindow(id, std::move(options))
    , assets_(assets)
    , ipc_(ipc) {}

WinBrowserWindow::~WinBrowserWindow() {
    shutting_down_ = true;
    ipc_.DetachWindow(id_);

    if (webview_) {
        ComPtr<ICoreWebView2_2> webview2;
        if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview2)))) {
            webview2->remove_WebResourceRequested(tokens_.web_resource_requested);
            webview2->remove_WebMessageReceived(tokens_.web_message_received);
        }
        webview_->remove_NavigationCompleted(tokens_.navigation_completed);
    }

    if (controller_) {
        controller_->Release();
        controller_ = nullptr;
    }
    if (webview_) {
        webview_->Release();
        webview_ = nullptr;
    }
    if (environment_) {
        environment_->Release();
        environment_ = nullptr;
    }
}

void WinBrowserWindow::Initialize() {
    if (!CreateWindowHandle()) {
        MessageBoxW(nullptr, L"Failed to create the application window.", L"Kutie", MB_OK | MB_ICONERROR);
        NotifyDestroyed();
        return;
    }

    if (options_.show) {
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
        visible_requested_ = true;
    }

    InitializeWebView();
}

void WinBrowserWindow::Close() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

void WinBrowserWindow::ExecuteScript(const std::string& script) {
    if (!hwnd_ || !webview_ || !webview_ready_ || shutting_down_) {
        return;
    }

    DWORD window_thread = 0;
    GetWindowThreadProcessId(hwnd_, &window_thread);
    if (window_thread != 0 && GetCurrentThreadId() != window_thread) {
        auto* queued_script = new (std::nothrow) std::string(script);
        if (queued_script == nullptr) {
            return;
        }
        if (!PostMessageW(hwnd_, WM_KUTIE_EXECUTE_SCRIPT, 0, reinterpret_cast<LPARAM>(queued_script))) {
            delete queued_script;
        }
        return;
    }

    webview_->ExecuteScript(platform::windows::Utf8ToWide(script).c_str(), nullptr);
}

void WinBrowserWindow::Show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
        visible_requested_ = true;
    }
}

void WinBrowserWindow::Hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void WinBrowserWindow::Focus() {
    if (hwnd_) {
        SetForegroundWindow(hwnd_);
    }
}

void WinBrowserWindow::SetTitle(const std::string& title) {
    options_.title = title;
    if (hwnd_) {
        SetWindowTextW(hwnd_, platform::windows::Utf8ToWide(title).c_str());
    }
}

void WinBrowserWindow::SetSize(int width, int height) {
    if (!hwnd_) {
        return;
    }
    options_.width = width;
    options_.height = height;
    const DWORD style = GetWindowLongW(hwnd_, GWL_STYLE);
    RECT rect{0, 0, width, height};
    AdjustWindowRect(&rect, style, FALSE);
    SetWindowPos(
        hwnd_,
        nullptr,
        0,
        0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOMOVE | SWP_NOZORDER);
}

void WinBrowserWindow::SetPosition(int x, int y) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void WinBrowserWindow::SetResizable(bool resizable) {
    options_.resizable = resizable;
    ScheduleApplyWindowStyle();
}

void WinBrowserWindow::SetFrame(bool frame) {
    options_.frame = frame;
    ScheduleApplyWindowStyle();
}

void WinBrowserWindow::Minimize() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_MINIMIZE, 0, 0);
    }
}

void WinBrowserWindow::Maximize() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_MAXIMIZE, 0, 0);
    }
}

void WinBrowserWindow::Restore() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_RESTORE, 0, 0);
    }
}

void WinBrowserWindow::ToggleMaximize() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_TOGGLE_MAXIMIZE, 0, 0);
    }
}

void WinBrowserWindow::StartDrag() {
    if (!hwnd_) {
        return;
    }
    PostMessageW(hwnd_, WM_KUTIE_START_DRAG, 0, 0);
}

void WinBrowserWindow::ToggleDevTools() {
    if (!webview_) {
        return;
    }
    ComPtr<ICoreWebView2_13> webview13;
    if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview13)))) {
        webview13->OpenDevToolsWindow();
    }
}

void WinBrowserWindow::OnClose(CloseHandler handler) {
    BrowserWindow::OnClose(std::move(handler));
}

void WinBrowserWindow::OnResize(ResizeHandler handler) {
    BrowserWindow::OnResize(std::move(handler));
}

void* WinBrowserWindow::NativeHandle() const {
    return hwnd_;
}

bool WinBrowserWindow::CreateWindowHandle() {
    static bool window_class_registered = false;
    if (!window_class_registered) {
        RegisterWindowClass();
        window_class_registered = true;
    }

    HWND owner = nullptr;
    HWND parent_hwnd = ResolveOwnerHwnd(options_.parent_id);
    if (options_.modal && parent_hwnd) {
        modal_parent_hwnd_ = parent_hwnd;
        owner = parent_hwnd;
    } else if (parent_hwnd && !options_.show_in_taskbar) {
        owner = parent_hwnd;
    }

    const UINT dpi = platform::windows::GetSystemDpi();
    const int width = MulDiv(options_.width, dpi, 96);
    const int height = MulDiv(options_.height, dpi, 96);

    DWORD style = platform::windows::BuildBaseWindowStyle(options_);
    DWORD ex_style = options_.always_on_top ? WS_EX_TOPMOST : 0;
    if (!owner) {
        ex_style |= WS_EX_APPWINDOW;
    }
    RECT rect{0, 0, width, height};

    using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    if (const auto adjust = reinterpret_cast<AdjustWindowRectExForDpiFn>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"))) {
        adjust(&rect, style, FALSE, ex_style, dpi);
    } else {
        AdjustWindowRectEx(&rect, style, FALSE, ex_style);
    }

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    if (options_.center) {
        const int screen_w = GetSystemMetrics(SM_CXSCREEN);
        const int screen_h = GetSystemMetrics(SM_CYSCREEN);
        x = (screen_w - (rect.right - rect.left)) / 2;
        y = (screen_h - (rect.bottom - rect.top)) / 2;
    }

    hwnd_ = CreateWindowExW(
        ex_style,
        kWindowClassName,
        platform::windows::Utf8ToWide(options_.title).c_str(),
        style,
        x,
        y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        owner,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (!hwnd_) {
        return false;
    }

    if (!options_.frame) {
        ApplyWindowStyle();
    } else {
        platform::windows::ApplyFramelessDwmChrome(hwnd_, options_);
    }

    if (parent_hwnd && !owner) {
        SetWindowLongPtrW(hwnd_, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(parent_hwnd));
    }

    if (options_.modal && modal_parent_hwnd_) {
        EnableWindow(modal_parent_hwnd_, FALSE);
        modal_locked_ = true;
    }

    return true;
}

void WinBrowserWindow::InitializeWebView() {
    WinWebViewHost::Instance().EnsureReady([this](HRESULT result, ICoreWebView2Environment* environment) {
        if (FAILED(result) || !environment || !hwnd_) {
            ShowStartupError(L"WebView2 environment failed to initialize.");
            return;
        }

        environment_ = environment;
        environment_->AddRef();

        environment->CreateCoreWebView2Controller(
            hwnd_,
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT controller_result, ICoreWebView2Controller* controller) -> HRESULT {
                    if (FAILED(controller_result)) {
                        ShowStartupError(L"WebView2 controller failed to initialize.");
                        return controller_result;
                    }

                    controller_ = controller;
                    controller_->AddRef();
                    controller_->get_CoreWebView2(&webview_);
                    if (webview_) {
                        webview_->AddRef();
                    }

                    UpdateWebViewBounds();
                    ConfigureResourceServing();
                    ConfigureIpcBridge();

                    webview_->Navigate(platform::windows::Utf8ToWide(options_.url).c_str());
                    webview_->add_NavigationCompleted(
                        Callback<ICoreWebView2NavigationCompletedEventHandler>(
                            [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                BOOL success = FALSE;
                                if (args) {
                                    args->get_IsSuccess(&success);
                                }
                                if (!success) {
                                    ShowStartupError(L"Failed to load the application UI.");
                                }
                                UpdateWebViewBounds();
                                ShowWhenReady();
                                return S_OK;
                            })
                            .Get(),
                        &tokens_.navigation_completed);

                    webview_ready_ = true;
                    return S_OK;
                })
                .Get());
    });
}

void WinBrowserWindow::ReleaseModalLock() {
    if (modal_locked_ && modal_parent_hwnd_) {
        EnableWindow(modal_parent_hwnd_, TRUE);
        SetForegroundWindow(modal_parent_hwnd_);
        modal_locked_ = false;
        modal_parent_hwnd_ = nullptr;
    }
}

void WinBrowserWindow::ScheduleApplyWindowStyle() {
    if (!hwnd_) {
        return;
    }
    PostMessageW(hwnd_, WM_KUTIE_APPLY_STYLE, 0, 0);
}

void WinBrowserWindow::ApplyWindowStyle() {
    if (!hwnd_) {
        return;
    }

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_STYLE));
    style = platform::windows::MergeWindowStyle(style, options_);
    SetWindowLongPtrW(hwnd_, GWL_STYLE, static_cast<LONG_PTR>(style));
    SetWindowPos(
        hwnd_,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    platform::windows::ApplyFramelessDwmChrome(hwnd_, options_);
}

void WinBrowserWindow::UpdateWebViewBounds() {
    if (!controller_ || !hwnd_) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    controller_->put_Bounds(client);
}

void WinBrowserWindow::ConfigureResourceServing() {
    if (!webview_) {
        return;
    }

    std::wstring folder = ResolveFrontendFolder();
    if (folder.empty()) {
        folder = MakeFallbackFolder(assets_);
    }

    ComPtr<ICoreWebView2_3> webview3;
    if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview3)))) {
        // When embedded assets exist, serve exclusively via WebResourceRequested below.
        if (assets_.Empty() && !folder.empty()) {
            webview3->SetVirtualHostNameToFolderMapping(
                kVirtualHost,
                folder.c_str(),
                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        }
    }

    ComPtr<ICoreWebView2_2> webview2;
    if (FAILED(webview_->QueryInterface(IID_PPV_ARGS(&webview2)))) {
        return;
    }

    webview2->AddWebResourceRequestedFilter(
        L"https://assets.kutie/*",
        COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

    webview2->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [this](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                ComPtr<ICoreWebView2WebResourceRequest> request;
                args->get_Request(&request);

                LPWSTR uri_wide = nullptr;
                request->get_Uri(&uri_wide);
                const std::string uri = platform::windows::WideToUtf8(uri_wide);
                CoTaskMemFree(uri_wide);

                if (uri.rfind(kVirtualOrigin, 0) != 0) {
                    return S_OK;
                }

                std::string path = uri.substr(sizeof(kVirtualOrigin) - 1);
                const auto query = path.find('?');
                if (query != std::string::npos) {
                    path = path.substr(0, query);
                }
                const auto hash = path.find('#');
                if (hash != std::string::npos) {
                    path = path.substr(0, hash);
                }
                path = AssetBundle::NormalizePath(path.empty() ? "/" : path);

                AssetBundle::Asset asset;
                if (!assets_.Resolve(path, asset)) {
                    const bool is_html =
                        path.size() >= 5 && _stricmp(path.c_str() + path.size() - 5, ".html") == 0;
                    if (!is_html && !AssetBundle::LooksLikeStaticAsset(path) &&
                        assets_.Resolve("/index.html", asset)) {
                        // SPA client-route fallback (not for missing .html files).
                    } else {
                        return S_OK;
                    }
                }

                HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, asset.bytes.size());
                if (!global) {
                    return E_OUTOFMEMORY;
                }

                void* locked = GlobalLock(global);
                memcpy(locked, asset.bytes.data(), asset.bytes.size());
                GlobalUnlock(global);

                ComPtr<IStream> stream;
                CreateStreamOnHGlobal(global, TRUE, &stream);

                const std::wstring headers =
                    L"Content-Type: " + platform::windows::Utf8ToWide(asset.content_type) +
                    L"\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-cache";

                ComPtr<ICoreWebView2WebResourceResponse> response;
                if (environment_) {
                    environment_->CreateWebResourceResponse(stream.Get(), 200, L"OK", headers.c_str(), &response);
                    args->put_Response(response.Get());
                }
                return S_OK;
            })
            .Get(),
        &tokens_.web_resource_requested);
}

void WinBrowserWindow::ConfigureIpcBridge() {
    if (!webview_) {
        return;
    }

    ipc_.AttachWindow(id_, [this](const std::string& script) { ExecuteScript(script); });

    ComPtr<ICoreWebView2_2> webview2;
    if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview2)))) {
        webview2->add_WebMessageReceived(
            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                    LPWSTR message_wide = nullptr;
                    args->get_WebMessageAsJson(&message_wide);
                    const std::string message_json = platform::windows::WideToUtf8(message_wide);
                    CoTaskMemFree(message_wide);

                    try {
                        nlohmann::json parsed = nlohmann::json::parse(message_json);
                        if (parsed.is_string()) {
                            parsed = nlohmann::json::parse(parsed.get<std::string>());
                        }

                        if (parsed.value("v", 0) != 1 || parsed.value("type", "") != "call") {
                            return S_OK;
                        }

                        const int call_id = parsed.value("id", 0);
                        const std::string name = parsed.value("name", "");
                        const std::string payload = parsed.contains("payload") ? parsed["payload"].dump() : "{}";
                        const std::string reply = ipc_.DispatchCall(id_, call_id, name, payload);
                        sender->PostWebMessageAsJson(platform::windows::Utf8ToWide(reply).c_str());
                    } catch (...) {
                    }
                    return S_OK;
                })
                .Get(),
            &tokens_.web_message_received);
    }

    ComPtr<ICoreWebView2_5> webview5;
    if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview5)))) {
        const std::wstring bootstrap = platform::windows::Utf8ToWide(IpcHub::ClientBootstrapScript(id_));
        webview5->AddScriptToExecuteOnDocumentCreated(bootstrap.c_str(), nullptr);
    } else {
        webview_->add_NavigationCompleted(
            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                    const std::wstring bootstrap = platform::windows::Utf8ToWide(IpcHub::ClientBootstrapScript(id_));
                    sender->ExecuteScript(bootstrap.c_str(), nullptr);
                    return S_OK;
                })
                .Get(),
            &tokens_.bootstrap_fallback);
    }
}

void WinBrowserWindow::ShowWhenReady() {
    if (!hwnd_ || !options_.show) {
        return;
    }
    if (!visible_requested_) {
        visible_requested_ = true;
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
    }
}

void WinBrowserWindow::ShowStartupError(const wchar_t* message) {
    if (!hwnd_) {
        return;
    }
    ShowWhenReady();
    MessageBoxW(hwnd_, message, L"Kutie", MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK WinBrowserWindow::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    WinBrowserWindow* window = nullptr;

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        window = reinterpret_cast<WinBrowserWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<WinBrowserWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!window) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_SIZE: {
        LRESULT size_result = 0;
        if (window->options_.frame) {
            size_result = DefWindowProcW(hwnd, message, wparam, lparam);
        } else {
            platform::windows::ApplyFramelessDwmChrome(hwnd, window->options_);
        }
        window->UpdateWebViewBounds();
        if (window->on_resize_ && wparam != SIZE_MINIMIZED) {
            RECT client{};
            GetClientRect(hwnd, &client);
            window->on_resize_(client.right, client.bottom);
        }
        if (window->options_.frame &&
            (wparam == SIZE_MAXIMIZED || wparam == SIZE_RESTORED)) {
            SetWindowPos(
                hwnd,
                nullptr,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            window->UpdateWebViewBounds();
        }
        return size_result;
    }

    case WM_DPICHANGED: {
        const RECT* suggested = reinterpret_cast<RECT*>(lparam);
        if (suggested) {
            SetWindowPos(
                hwnd,
                nullptr,
                suggested->left,
                suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (window->controller_) {
            window->UpdateWebViewBounds();
        }
        return 0;
    }

    case WM_GETMINMAXINFO: {
        auto* minmax = reinterpret_cast<MINMAXINFO*>(lparam);
        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(monitor_info);
        const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (GetMonitorInfoW(monitor, &monitor_info)) {
            platform::windows::AdjustMinMaxInfoForWorkArea(
                minmax,
                monitor_info.rcWork,
                monitor_info.rcMonitor);
        }
        // Owned/modal windows need explicit work-area limits; DefWindowProc can clip the caption.
        return 0;
    }

    case WM_NCCALCSIZE: {
        const bool maximized = IsZoomed(hwnd) != FALSE;
        if (const auto result = platform::windows::HandleFramelessNcCalcSize(
                hwnd, wparam, lparam, window->options_, maximized)) {
            window->UpdateWebViewBounds();
            return *result;
        }
        break;
    }

    case WM_WINDOWPOSCHANGED:
        window->UpdateWebViewBounds();
        break;

    case WM_KUTIE_APPLY_STYLE:
        window->ApplyWindowStyle();
        window->UpdateWebViewBounds();
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;

    case WM_KUTIE_START_DRAG:
        ReleaseCapture();
        SendMessageW(hwnd, WM_SYSCOMMAND, platform::windows::kSysCommandDragMove, 0);
        return 0;

    case WM_KUTIE_MINIMIZE:
        ShowWindow(hwnd, SW_MINIMIZE);
        return 0;

    case WM_KUTIE_MAXIMIZE:
        ShowWindow(hwnd, SW_MAXIMIZE);
        return 0;

    case WM_KUTIE_RESTORE:
        ShowWindow(hwnd, SW_RESTORE);
        return 0;

    case WM_KUTIE_TOGGLE_MAXIMIZE:
        ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
        return 0;

    case WM_KUTIE_EXECUTE_SCRIPT: {
        auto* script = reinterpret_cast<std::string*>(lparam);
        if (script != nullptr) {
            if (window->webview_ && window->webview_ready_ && !window->shutting_down_) {
                window->webview_->ExecuteScript(
                    platform::windows::Utf8ToWide(*script).c_str(), nullptr);
            }
            delete script;
        }
        return 0;
    }

    case WM_CLOSE:
        if (window->on_close_ && !window->on_close_()) {
            return 0;
        }
        break;

    case WM_DESTROY:
        window->shutting_down_ = true;
        window->ReleaseModalLock();
        window->NotifyDestroyed();
        if (BrowserWindow::GetAll().empty()) {
            PostQuitMessage(0);
        }
        return 0;

    case WM_KEYDOWN:
        if (wparam == VK_F12 && window->options_.devtools) {
            window->ToggleDevTools();
        }
        return 0;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace kutie
