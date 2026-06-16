#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace kutie {

struct BrowserWindowOptions {
    std::string title = "Kutie App";
    int width = 1024;
    int height = 768;
    bool center = true;
    bool resizable = true;
    bool always_on_top = false;
    bool show = true;
    bool frame = true;
    bool shadow = true;
    bool devtools = false;
    std::string url = "https://assets.kutie/index.html";

    uint32_t parent_id = 0;
    bool modal = false;
};

class BrowserWindow {
public:
    using CloseHandler = std::function<bool()>;
    using ResizeHandler = std::function<void(int width, int height)>;

    static BrowserWindow& Create(const BrowserWindowOptions& options);
    static BrowserWindow* FromId(uint32_t id);
    static BrowserWindow* GetFocused();
    static std::vector<BrowserWindow*> GetAll();
    static void FlushPendingDeletes();

    virtual ~BrowserWindow();

    uint32_t Id() const { return id_; }
    std::optional<uint32_t> ParentId() const;
    bool IsModal() const { return options_.modal; }

    virtual void Close();
    virtual void Show();
    virtual void Hide();
    virtual void Focus();
    virtual void SetTitle(const std::string& title);
    virtual void SetSize(int width, int height);
    virtual void SetPosition(int x, int y);
    virtual void SetResizable(bool resizable);
    virtual void SetFrame(bool frame);
    virtual void Minimize();
    virtual void Maximize();
    virtual void Restore();
    virtual void ToggleMaximize();
    virtual void StartDrag();
    virtual void ToggleDevTools();
    virtual void ExecuteScript(const std::string& script);

    virtual void OnClose(CloseHandler handler);
    virtual void OnResize(ResizeHandler handler);
    virtual void* NativeHandle() const;

    const BrowserWindowOptions& Options() const { return options_; }

protected:
    BrowserWindow(uint32_t id, BrowserWindowOptions options);

    void NotifyDestroyed();

    uint32_t id_;
    BrowserWindowOptions options_;
    CloseHandler on_close_;
    ResizeHandler on_resize_;

private:
    static uint32_t AllocateId();
    static void RegisterWindow(uint32_t id, std::unique_ptr<BrowserWindow> window);
};

std::string ValidateBrowserWindowOptions(const BrowserWindowOptions& options);

} // namespace kutie
