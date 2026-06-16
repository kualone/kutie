#pragma once

#include "kutie/asset_bundle.hpp"
#include "kutie/browser_window.hpp"
#include "kutie/ipc_hub.hpp"

#include <WebView2.h>
#include <atomic>

namespace kutie {

class WinBrowserWindow final : public BrowserWindow {
public:
    WinBrowserWindow(
        uint32_t id,
        BrowserWindowOptions options,
        AssetBundle& assets,
        IpcHub& ipc);
    ~WinBrowserWindow() override;

    void Initialize();

    void Close() override;
    void Show() override;
    void Hide() override;
    void Focus() override;
    void SetTitle(const std::string& title) override;
    void SetSize(int width, int height) override;
    void SetPosition(int x, int y) override;
    void SetResizable(bool resizable) override;
    void SetFrame(bool frame) override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    void ToggleMaximize() override;
    void StartDrag() override;
    void ToggleDevTools() override;
    void ExecuteScript(const std::string& script) override;
    void OnClose(CloseHandler handler) override;
    void OnResize(ResizeHandler handler) override;
    void* NativeHandle() const override;

private:
    struct EventTokens {
        EventRegistrationToken web_resource_requested{};
        EventRegistrationToken web_message_received{};
        EventRegistrationToken navigation_completed{};
        EventRegistrationToken bootstrap_fallback{};
    };

    bool CreateWindowHandle();
    void InitializeWebView();
    void ApplyWindowStyle();
    void ScheduleApplyWindowStyle();
    void UpdateWebViewBounds();
    void ConfigureResourceServing();
    void ConfigureIpcBridge();
    void ShowWhenReady();
    void ShowStartupError(const wchar_t* message);
    void ReleaseModalLock();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static void RegisterWindowClass();

    AssetBundle& assets_;
    IpcHub& ipc_;

    HWND hwnd_ = nullptr;
    HWND modal_parent_hwnd_ = nullptr;
    ICoreWebView2* webview_ = nullptr;
    ICoreWebView2Controller* controller_ = nullptr;
    ICoreWebView2Environment* environment_ = nullptr;

    EventTokens tokens_;
    std::atomic<bool> shutting_down_{false};
    bool webview_ready_ = false;
    bool visible_requested_ = false;
    bool modal_locked_ = false;
};

} // namespace kutie
