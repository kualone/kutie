#pragma once

#include "types.hpp"
#include <functional>
#include <memory>
#include <string>

namespace kutie {

class AssetBundle;
class IpcHub;

struct ShellConfig {
    std::string title = "Kutie App";
    int width = 1024;
    int height = 768;
    bool center = true;
    bool resizable = true;
    bool always_on_top = false;
    bool devtools = false;
    bool decorations = true;
    bool shadow = true;
    Color background{};
    std::string entry_url = "https://assets.kutie/index.html";
    TitleBarMode title_bar_mode = TitleBarMode::Native;
};

class IShell {
public:
    using CloseHandler = std::function<bool()>;
    using ResizeHandler = std::function<void(int width, int height)>;
    using StateHandler = std::function<void()>;

    virtual ~IShell() = default;

    virtual int Run() = 0;
    virtual void Close() = 0;
    virtual void ExecuteScript(const std::string& script) = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetSize(int width, int height) = 0;
    virtual void SetPosition(int x, int y) = 0;
    virtual void SetAlwaysOnTop(bool on_top) = 0;
    virtual void SetResizable(bool resizable) = 0;
    virtual void SetDecorations(bool decorations) = 0;
    virtual void SetBackground(const Color& background) = 0;
    virtual void SetIcon(void* icon_handle) = 0;
    virtual void Minimize() = 0;
    virtual void Maximize() = 0;
    virtual void Restore() = 0;
    virtual void ToggleMaximize() = 0;
    virtual void StartDrag() = 0;
    virtual void ToggleDevTools() = 0;

    virtual bool IsMinimized() const = 0;
    virtual bool IsMaximized() const = 0;
    virtual bool IsFocused() const = 0;

    virtual void OnClose(CloseHandler handler) = 0;
    virtual void OnResize(ResizeHandler handler) = 0;
    virtual void OnMinimize(StateHandler handler) = 0;
    virtual void OnMaximize(StateHandler handler) = 0;
    virtual void OnFocus(StateHandler handler) = 0;

    virtual void* NativeHandle() const = 0;
};

} // namespace kutie
