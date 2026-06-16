#include "win_shell.hpp"

#include "win_dpi.hpp"
#include "win_frameless.hpp"
#include "win_string_util.hpp"

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

constexpr wchar_t kWindowClassName[] = L"KutieShellWindow";
constexpr wchar_t kVirtualHost[] = L"assets.kutie";
constexpr char kVirtualOrigin[] = "https://assets.kutie";

constexpr UINT WM_KUTIE_APPLY_STYLE = WM_USER + 1;
constexpr UINT WM_KUTIE_START_DRAG = WM_USER + 2;
constexpr UINT WM_KUTIE_MINIMIZE = WM_USER + 3;
constexpr UINT WM_KUTIE_MAXIMIZE = WM_USER + 4;
constexpr UINT WM_KUTIE_RESTORE = WM_USER + 5;
constexpr UINT WM_KUTIE_TOGGLE_MAXIMIZE = WM_USER + 6;
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
    WCHAR exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    WCHAR* slash = wcsrchr(exe_path, L'\\');
    if (slash) {
        *slash = L'\0';
    }

    const std::vector<std::wstring> candidates = {
        std::wstring(exe_path) + L"\\..\\..\\sample\\frontend",
        std::wstring(exe_path) + L"\\frontend",
    };

    for (const auto& candidate : candidates) {
        WCHAR absolute[MAX_PATH];
        if (GetFullPathNameW(candidate.c_str(), MAX_PATH, absolute, nullptr)) {
            if (GetFileAttributesW(absolute) != INVALID_FILE_ATTRIBUTES) {
                return absolute;
            }
        }
    }
    return {};
}

} // namespace

void WinShell::RegisterWindowClass() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = CreateSolidBrush(RGB(18, 16, 32));
    window_class.lpszClassName = kWindowClassName;
    RegisterClassExW(&window_class);
}

WinShell::WinShell(const ShellConfig& config, AssetBundle& assets, IpcHub& ipc)
    : config_(config)
    , assets_(assets)
    , ipc_(ipc) {
}

WinShell::~WinShell() {
    shutting_down_ = true;

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

int WinShell::Run() {
    platform::windows::EnsurePerMonitorDpiAwareness();

    const HRESULT com_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool com_initialized = SUCCEEDED(com_hr) || com_hr == RPC_E_CHANGED_MODE;

    if (!CreateWindowHandle()) {
        MessageBoxW(nullptr, L"Failed to create the application window.", L"Kutie", MB_OK | MB_ICONERROR);
        if (com_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    // Show immediately so users see the themed background while WebView2 initializes.
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    visible_requested_ = true;

    if (!InitializeWebView()) {
        MessageBoxW(
            hwnd_,
            L"Failed to start WebView2. Install the Microsoft Edge WebView2 Runtime and try again.",
            L"Kutie",
            MB_OK | MB_ICONERROR);
        if (com_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (com_initialized) {
        CoUninitialize();
    }
    return static_cast<int>(message.wParam);
}

void WinShell::Close() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

void WinShell::ExecuteScript(const std::string& script) {
    if (!webview_ || !webview_ready_ || shutting_down_) {
        return;
    }
    webview_->ExecuteScript(platform::windows::Utf8ToWide(script).c_str(), nullptr);
}

void WinShell::SetTitle(const std::string& title) {
    config_.title = title;
    if (hwnd_) {
        SetWindowTextW(hwnd_, platform::windows::Utf8ToWide(title).c_str());
    }
}

void WinShell::SetSize(int width, int height) {
    if (!hwnd_) {
        return;
    }
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

void WinShell::SetPosition(int x, int y) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void WinShell::SetAlwaysOnTop(bool on_top) {
    config_.always_on_top = on_top;
    if (hwnd_) {
        SetWindowPos(
            hwnd_,
            on_top ? HWND_TOPMOST : HWND_NOTOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE);
    }
}

void WinShell::SetResizable(bool resizable) {
    config_.resizable = resizable;
    ScheduleApplyWindowStyle();
}

void WinShell::SetDecorations(bool decorations) {
    config_.decorations = decorations;
    ScheduleApplyWindowStyle();
}

void WinShell::SetBackground(const Color& background) {
    config_.background = background;
    ApplyShellBackground();
}

void WinShell::ApplyShellBackground() {
    ApplyWebViewBackground();
    if (hwnd_) {
        platform::windows::ApplyFramelessDwmChrome(hwnd_, config_);
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void WinShell::SetIcon(void* icon_handle) {
    if (hwnd_ && icon_handle) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon_handle));
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon_handle));
    }
}

void WinShell::Minimize() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_MINIMIZE, 0, 0);
    }
}

void WinShell::Maximize() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_MAXIMIZE, 0, 0);
    }
}

void WinShell::Restore() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_RESTORE, 0, 0);
    }
}

void WinShell::ToggleMaximize() {
    if (hwnd_) {
        PostMessageW(hwnd_, WM_KUTIE_TOGGLE_MAXIMIZE, 0, 0);
    }
}

void WinShell::StartDrag() {
    if (!hwnd_) {
        return;
    }
    PostMessageW(hwnd_, WM_KUTIE_START_DRAG, 0, 0);
}

void WinShell::ToggleDevTools() {
    if (!webview_) {
        return;
    }
    ComPtr<ICoreWebView2_13> webview13;
    if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview13)))) {
        webview13->OpenDevToolsWindow();
    }
}

bool WinShell::IsMinimized() const {
    return hwnd_ && IsIconic(hwnd_);
}

bool WinShell::IsMaximized() const {
    return hwnd_ && IsZoomed(hwnd_);
}

bool WinShell::IsFocused() const {
    return hwnd_ && GetForegroundWindow() == hwnd_;
}

void WinShell::OnClose(CloseHandler handler) { on_close_ = std::move(handler); }
void WinShell::OnResize(ResizeHandler handler) { on_resize_ = std::move(handler); }
void WinShell::OnMinimize(StateHandler handler) { on_minimize_ = std::move(handler); }
void WinShell::OnMaximize(StateHandler handler) { on_maximize_ = std::move(handler); }
void WinShell::OnFocus(StateHandler handler) { on_focus_ = std::move(handler); }

void* WinShell::NativeHandle() const {
    return hwnd_;
}

bool WinShell::CreateWindowHandle() {
    static bool window_class_registered = false;
    if (!window_class_registered) {
        RegisterWindowClass();
        window_class_registered = true;
    }

    const UINT dpi = platform::windows::GetSystemDpi();
    const int width = MulDiv(config_.width, dpi, 96);
    const int height = MulDiv(config_.height, dpi, 96);

    DWORD style = WS_OVERLAPPED | platform::windows::BuildDecorationStyle(config_);

    DWORD ex_style = config_.always_on_top ? WS_EX_TOPMOST : 0;
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
    if (config_.center) {
        const int screen_w = GetSystemMetrics(SM_CXSCREEN);
        const int screen_h = GetSystemMetrics(SM_CYSCREEN);
        x = (screen_w - (rect.right - rect.left)) / 2;
        y = (screen_h - (rect.bottom - rect.top)) / 2;
    }

    hwnd_ = CreateWindowExW(
        ex_style,
        kWindowClassName,
        platform::windows::Utf8ToWide(config_.title).c_str(),
        style,
        x,
        y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (!hwnd_) {
        return false;
    }

    platform::windows::ApplyFramelessDwmChrome(hwnd_, config_);

    return hwnd_ != nullptr;
}

bool WinShell::InitializeWebView() {
    WCHAR temp_dir[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_dir);
    const std::wstring user_data = std::wstring(temp_dir) + L"kutie_webview2_" + std::to_wstring(GetCurrentProcessId());

    const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        user_data.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(result)) {
                    ShowStartupError(L"WebView2 environment failed to initialize.");
                    return result;
                }

                environment_ = environment;
                environment_->AddRef();

                return environment->CreateCoreWebView2Controller(
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

                            ApplyWebViewBackground();
                            ConfigureResourceServing();
                            ConfigureIpcBridge();

                            webview_->Navigate(platform::windows::Utf8ToWide(config_.entry_url).c_str());
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
            })
            .Get());

    return SUCCEEDED(hr);
}

void WinShell::ScheduleApplyWindowStyle() {
    if (!hwnd_) {
        return;
    }
    // Defer style changes out of the WebView2 IPC callback stack.
    PostMessageW(hwnd_, WM_KUTIE_APPLY_STYLE, 0, 0);
}

void WinShell::ApplyWindowStyle() {
    if (!hwnd_) {
        return;
    }

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_STYLE));
    style = platform::windows::MergeWindowStyle(style, config_);
    SetWindowLongPtrW(hwnd_, GWL_STYLE, static_cast<LONG_PTR>(style));
    SetWindowPos(
        hwnd_,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    platform::windows::ApplyFramelessDwmChrome(hwnd_, config_);
}

void WinShell::UpdateWebViewBounds() {
    if (!controller_ || !hwnd_) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    controller_->put_Bounds(client);
}

void WinShell::ApplyWebViewBackground() {
    if (!controller_) {
        return;
    }

    ComPtr<ICoreWebView2Controller2> controller2;
    if (SUCCEEDED(controller_->QueryInterface(IID_PPV_ARGS(&controller2)))) {
        COREWEBVIEW2_COLOR color{};
        color.A = config_.background.a;
        color.R = config_.background.r;
        color.G = config_.background.g;
        color.B = config_.background.b;
        controller2->put_DefaultBackgroundColor(color);
    }
}

void WinShell::ConfigureResourceServing() {
    if (!webview_) {
        return;
    }

    std::wstring folder = ResolveFrontendFolder();
    if (folder.empty()) {
        folder = MakeFallbackFolder(assets_);
    }

    ComPtr<ICoreWebView2_3> webview3;
    if (SUCCEEDED(webview_->QueryInterface(IID_PPV_ARGS(&webview3)))) {
        webview3->SetVirtualHostNameToFolderMapping(
            kVirtualHost,
            folder.c_str(),
            COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
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
                    if (!AssetBundle::LooksLikeStaticAsset(path) && assets_.Resolve("/index.html", asset)) {
                        // SPA fallback
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

void WinShell::ConfigureIpcBridge() {
    if (!webview_) {
        return;
    }

    ipc_.SetScriptRunner([this](const std::string& script) { ExecuteScript(script); });

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

                        const int id = parsed.value("id", 0);
                        const std::string name = parsed.value("name", "");
                        const std::string payload = parsed.contains("payload") ? parsed["payload"].dump() : "{}";
                        const std::string reply = ipc_.DispatchCall(id, name, payload);
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
        const std::wstring bootstrap = platform::windows::Utf8ToWide(IpcHub::ClientBootstrapScript());
        webview5->AddScriptToExecuteOnDocumentCreated(bootstrap.c_str(), nullptr);
    } else {
        webview_->add_NavigationCompleted(
            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                    const std::wstring bootstrap = platform::windows::Utf8ToWide(IpcHub::ClientBootstrapScript());
                    sender->ExecuteScript(bootstrap.c_str(), nullptr);
                    return S_OK;
                })
                .Get(),
            &tokens_.bootstrap_fallback);
    }
}

void WinShell::ShowWhenReady() {
    if (!hwnd_) {
        return;
    }
    if (!visible_requested_) {
        visible_requested_ = true;
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
    }
}

void WinShell::ShowStartupError(const wchar_t* message) {
    if (!hwnd_) {
        return;
    }
    ShowWhenReady();
    MessageBoxW(hwnd_, message, L"Kutie", MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK WinShell::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    WinShell* shell = nullptr;

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        shell = reinterpret_cast<WinShell*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(shell));
    } else {
        shell = reinterpret_cast<WinShell*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!shell) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_SIZE:
        shell->UpdateWebViewBounds();
        if (shell->on_resize_ && wparam != SIZE_MINIMIZED) {
            RECT client{};
            GetClientRect(hwnd, &client);
            shell->on_resize_(client.right, client.bottom);
        }
        if (wparam == SIZE_MINIMIZED && shell->on_minimize_) {
            shell->on_minimize_();
        }
        if (wparam == SIZE_MAXIMIZED && shell->on_maximize_) {
            shell->on_maximize_();
        }
        return 0;

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
        if (shell->controller_) {
            shell->UpdateWebViewBounds();
        }
        return 0;
    }

    case WM_ACTIVATE:
        if (shell->on_focus_ && LOWORD(wparam) != WA_INACTIVE) {
            shell->on_focus_();
        }
        return 0;

    case WM_GETMINMAXINFO:
        if (!shell->config_.decorations) {
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
            return 0;
        }
        break;

    case WM_NCCALCSIZE: {
        const bool maximized = IsZoomed(hwnd) != FALSE;
        if (const auto result = platform::windows::HandleFramelessNcCalcSize(
                hwnd, wparam, lparam, shell->config_, maximized)) {
            return *result;
        }
        break;
    }

    case WM_KUTIE_APPLY_STYLE:
        shell->ApplyWindowStyle();
        shell->UpdateWebViewBounds();
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

    case WM_CLOSE:
        if (shell->on_close_ && !shell->on_close_()) {
            return 0;
        }
        break;

    case WM_DESTROY:
        shell->shutting_down_ = true;
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wparam == VK_F12 && shell->config_.devtools) {
            shell->ToggleDevTools();
        }
        return 0;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace kutie
