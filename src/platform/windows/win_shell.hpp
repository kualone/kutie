#pragma once

#include "kutie/asset_bundle.hpp"
#include "kutie/ipc_hub.hpp"
#include "kutie/shell.hpp"

#include <WebView2.h>
#include <atomic>
#include <memory>

namespace kutie {

class WinShell final : public IShell {
public:
    WinShell(const ShellConfig& config, AssetBundle& assets, IpcHub& ipc);
    ~WinShell() override;

    int Run() override;
    void Close() override;
    void ExecuteScript(const std::string& script) override;

    void SetTitle(const std::string& title) override;
    void SetSize(int width, int height) override;
    void SetPosition(int x, int y) override;
    void SetAlwaysOnTop(bool on_top) override;
    void SetResizable(bool resizable) override;
    void SetDecorations(bool decorations) override;
    void SetIcon(void* icon_handle) override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    void ToggleMaximize() override;
    void StartDrag() override;
    void ToggleDevTools() override;

    bool IsMinimized() const override;
    bool IsMaximized() const override;
    bool IsFocused() const override;

    void OnClose(CloseHandler handler) override;
    void OnResize(ResizeHandler handler) override;
    void OnMinimize(StateHandler handler) override;
    void OnMaximize(StateHandler handler) override;
    void OnFocus(StateHandler handler) override;

    void* NativeHandle() const override;

private:
    struct EventTokens {
        EventRegistrationToken web_resource_requested{};
        EventRegistrationToken web_message_received{};
        EventRegistrationToken navigation_completed{};
        EventRegistrationToken bootstrap_fallback{};
    };

    bool CreateWindowHandle();
    bool InitializeWebView();
    void ApplyWindowStyle();
    void ScheduleApplyWindowStyle();
    void UpdateWebViewBounds();
    void ApplyWebViewBackground();
    void ConfigureResourceServing();
    void ConfigureIpcBridge();
    void ShowWhenReady();
    void ShowStartupError(const wchar_t* message);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    ShellConfig config_;
    AssetBundle& assets_;
    IpcHub& ipc_;

    HWND hwnd_ = nullptr;
    ICoreWebView2* webview_ = nullptr;
    ICoreWebView2Controller* controller_ = nullptr;
    ICoreWebView2Environment* environment_ = nullptr;

    EventTokens tokens_;
    std::atomic<bool> shutting_down_{false};
    bool webview_ready_ = false;
    bool visible_requested_ = false;

    CloseHandler on_close_;
    ResizeHandler on_resize_;
    StateHandler on_minimize_;
    StateHandler on_maximize_;
    StateHandler on_focus_;
};

} // namespace kutie
