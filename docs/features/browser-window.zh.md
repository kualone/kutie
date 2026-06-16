# BrowserWindow

Kutie 提供类似 Electron 的 `BrowserWindow` API，用于创建和管理嵌入 WebView2 的原生窗口。

## 概览

| 层级 | API |
|---|---|
| C++ | `BrowserWindow::Create(options)`、实例方法、静态 `GetAll()` / `GetFocused()` / `FromId()` |
| JavaScript | `kutie.BrowserWindow.create()`、`getCurrent()`、`getAll()`、`getFocused()` |

每个窗口有进程内唯一的数字 `id`（与 Electron 类似）。IPC handler 全局共享；内置 `window.*` 调用会路由到发起请求的窗口。

## C++ 用法

```cpp
kutie::Runtime::Config config;
config.main_window.title = "My App";
config.main_window.url = "https://assets.kutie/index.html";

kutie::Runtime app(config);
app.OnReady([&](kutie::Runtime&) {
    const uint32_t main_id = kutie::BrowserWindow::GetAll().front()->Id();

    kutie::BrowserWindowOptions dialog;
    dialog.parent_id = main_id;
    dialog.modal = true;
    dialog.width = 480;
    dialog.height = 360;
    dialog.frame = true;
    dialog.url = "https://assets.kutie/dialog.html";
    kutie::BrowserWindow::Create(dialog);
});

return app.Run();
```

## JavaScript 用法

```javascript
const win = kutie.BrowserWindow.getCurrent();
await win.minimize();
await win.setFrame(false);

const modal = await kutie.BrowserWindow.create({
  title: 'Settings',
  url: 'https://assets.kutie/settings.html',
  width: 480,
  height: 360,
  parent_id: win.id,
  modal: true,
  frame: true,
});
await modal.close();
```

## BrowserWindowOptions

| 字段 | 默认值 | 说明 |
|---|---|---|
| `title` | `"Kutie App"` | 窗口标题 |
| `width` / `height` | `1024` / `768` | 初始尺寸（逻辑像素） |
| `center` | `true` | 创建时居中 |
| `resizable` | `true` | 是否可缩放 |
| `always_on_top` | `false` | 置顶 |
| `show` | `true` | WebView 就绪后显示 |
| `frame` | `true` | 原生标题栏（`false` = partial decoration / 自定义标题栏） |
| `shadow` | `true` | 窗口阴影（Windows） |
| `devtools` | `false` | 允许 F12 打开 DevTools |
| `url` | `https://assets.kutie/index.html` | 初始导航 URL |
| `parent_id` | `0` | 父窗口 id（`0` 表示无） |
| `modal` | `false` | 阻塞父窗口交互（需 `parent_id`） |
| `show_in_taskbar` | 无 parent 时为 `true`，否则默认 `false` | Windows 上是否显示独立任务栏按钮 |

## 子窗口自定义标题栏

`frame: false` 时原生标题栏被收入客户区（partial decoration）。**页面需提供 HTML 标题栏** — 参考 `sample/frontend/child-window.html`。否则最大化后无法操作最小化/最大化/关闭。

Sample 模态框（`sample/frontend/dialog.html`）使用 **`frame: true`**（原生 OS 标题栏），最大化/关闭由系统标题栏提供，无需 HTML 标题栏。

使用 `data-kutie-drag-region`，按钮调用 `kutie.BrowserWindow.getCurrent()`（见 `sample/frontend/scripts/child-window.js`）。

## 任务栏图标（Windows）

| 配置 | 任务栏 |
|---|---|
| 无 `parent_id` | 独立任务栏按钮（`WS_EX_APPWINDOW`） |
| 有 `parent_id` 且 `show_in_taskbar: false`（子窗口默认） | 无独立按钮（Win32 owner） |
| 有 `parent_id` 且 `show_in_taskbar: true` | 独立按钮；通过 `GWLP_HWNDPARENT` 保持层级 |
| `modal: true` | 永不单独出现在任务栏 |

## 模态窗口

当 `modal: true` 且设置了 `parent_id` 时：

- 子窗口以父窗口为 Win32 owner 创建。
- 父 HWND 通过 `EnableWindow(FALSE)` 禁用，直到模态窗口关闭。
- 销毁时重新启用并聚焦父窗口。

## Runtime 生命周期

`Runtime::Run()`：

1. 初始化 COM 与共享 WebView2 环境（`WinWebViewHost`）
2. 根据 `config.main_window` 创建主窗口
3. 调用 `OnReady`
4. 运行 Win32 消息循环，直到所有窗口关闭

## 边界情况

- **最后一个窗口关闭** → 消息循环退出（`PostQuitMessage`）。
- **id 重复** — 不允许；id 自动递增分配。
- **无 parent 的 modal** — 创建时 `ValidateBrowserWindowOptions` 会拒绝。
- **运行时 loadURL** — v1 不支持；创建窗口时在 options 中设置 `url`。

另见：[../custom-titlebar.zh.md](../custom-titlebar.zh.md)、[../ipc-protocol.zh.md](../ipc-protocol.zh.md)。
