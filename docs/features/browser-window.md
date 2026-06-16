# BrowserWindow

Kutie exposes an Electron-style `BrowserWindow` API for creating and managing native windows with embedded WebView2 content.

## Overview

| Layer | API |
|---|---|
| C++ | `BrowserWindow::Create(options)`, instance methods, static `GetAll()` / `GetFocused()` / `FromId()` |
| JavaScript | `kutie.BrowserWindow.create()`, `getCurrent()`, `getAll()`, `getFocused()` |

Each window has a process-wide numeric `id` (similar to Electron). IPC handlers are global; built-in `window.*` calls route to the window that initiated the request.

## C++ usage

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
    dialog.frame = false;
    dialog.url = "https://assets.kutie/dialog.html";
    kutie::BrowserWindow::Create(dialog);
});

return app.Run();
```

## JavaScript usage

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
  frame: false,
});
await modal.close();
```

## BrowserWindowOptions

| Field | Default | Description |
|---|---|---|
| `title` | `"Kutie App"` | Window title |
| `width` / `height` | `1024` / `768` | Initial size (logical px) |
| `center` | `true` | Center on screen when created |
| `resizable` | `true` | Allow resize |
| `always_on_top` | `false` | Topmost window |
| `show` | `true` | Show after WebView is ready |
| `frame` | `true` | Native title bar (`false` = partial decoration / custom titlebar) |
| `shadow` | `true` | Window shadow (Windows) |
| `devtools` | `false` | Allow F12 DevTools |
| `url` | `https://assets.kutie/index.html` | Initial navigation URL |
| `parent_id` | `0` | Parent window id (`0` = none) |
| `modal` | `false` | Block parent interaction (requires `parent_id`) |

## Modal windows

When `modal: true` and `parent_id` is set:

- The child window is created with the parent as Win32 owner.
- The parent HWND is disabled via `EnableWindow(FALSE)` until the modal closes.
- On destroy, the parent is re-enabled and focused.

## Runtime lifecycle

`Runtime::Run()`:

1. Initializes COM and shared WebView2 environment (`WinWebViewHost`)
2. Creates the main window from `config.main_window`
3. Invokes `OnReady`
4. Runs the Win32 message loop until all windows close

## Edge cases

- **Last window closes** → message loop exits (`PostQuitMessage`).
- **Duplicate ids** — not allowed; ids are auto-assigned.
- **Modal without parent** — `ValidateBrowserWindowOptions` rejects at create time.
- **Runtime `loadURL`** — not supported in v1; set `url` in options when creating the window.

See also: [../custom-titlebar.md](../custom-titlebar.md), [../ipc-protocol.md](../ipc-protocol.md).
