# API Reference

## Runtime

```cpp
kutie::Runtime::Config cfg;
cfg.main_window.title = "App";
cfg.main_window.width = 1024;
cfg.main_window.height = 768;
cfg.main_window.center = true;
cfg.main_window.resizable = true;
cfg.main_window.always_on_top = false;
cfg.main_window.devtools = true;
cfg.main_window.frame = true;
cfg.main_window.shadow = true;
cfg.main_window.url = "https://assets.kutie/index.html";

kutie::Runtime app(cfg);
app.OnReady([](kutie::Runtime& rt) { /* after main window is created */ });
int code = app.Run();

app.ipc();    // IpcHub&
app.assets(); // AssetBundle&
```

## BrowserWindow

```cpp
kutie::BrowserWindow& main = kutie::BrowserWindow::Create(options);
main.Close();
main.SetTitle("New title");
main.SetFrame(false);

kutie::BrowserWindow* win = kutie::BrowserWindow::FromId(1);
auto all = kutie::BrowserWindow::GetAll();
auto* focused = kutie::BrowserWindow::GetFocused();
```

| Method | Description |
|---|---|
| `Create(options)` | Create window; returns new instance |
| `FromId(id)` / `GetAll()` / `GetFocused()` | Registry queries |
| `Close()` | Request close |
| `Show()` / `Hide()` / `Focus()` | Visibility and focus |
| `ExecuteScript(js)` | Run JS in WebView |
| `SetTitle` / `SetSize` / `SetPosition` | Geometry |
| `SetResizable` / `SetFrame` | Window flags |
| `Minimize` / `Maximize` / `Restore` / `ToggleMaximize` | State |
| `StartDrag()` | Native drag via `WM_SYSCOMMAND SC_DRAGMOVE` |
| `ToggleDevTools()` | DevTools window |
| `OnClose` / `OnResize` | Lifecycle callbacks |
| `NativeHandle()` | `HWND` on Windows |

See [features/browser-window.md](features/browser-window.md).

## IpcHub

```cpp
ipc.RegisterHandler("name", handler);
ipc.UnregisterHandler("name");
ipc.Broadcast("event", nlohmann::json{{"key", "value"}});

// Typed helper
ipc.RegisterHandler<MyArgs, MyResult>("typed", fn);
```

## AssetBundle

```cpp
bundle.Put("/path/file.html", "<html></html>");
bundle.Get("/path/file.html");       // const Asset*
bundle.Resolve(path, out);           // copy
bundle.LoadEmbedded(module_handle);
bundle.LoadFromDisk("/path/to/frontend");
bundle.Paths();
bundle.Empty();
```

## PlatformServices

```cpp
PlatformServices::OpenFiles(hwnd, options);
PlatformServices::SaveFile(hwnd, options);
PlatformServices::PickFolder(hwnd, title);
PlatformServices::ReadClipboardText();
PlatformServices::WriteClipboardText(text);
PlatformServices::ClearClipboard();
PlatformServices::ShowInfo(hwnd, title, message);
PlatformServices::AskConfirm(hwnd, title, message);
```

## JavaScript (`window.kutie`)

```javascript
await kutie.call('handler.name', { key: 'value' });
const off = kutie.on('event.name', (data) => {});

const win = kutie.BrowserWindow.getCurrent();
await win.minimize();
await win.setFrame(false);
await kutie.BrowserWindow.create({ title: 'Dialog', url: '...', parent_id: win.id, modal: true });
```

## BrowserWindowOptions Fields

| Field | Default | Description |
|---|---|---|
| `title` | `"Kutie App"` | Window title |
| `width` / `height` | 1024 × 768 | Logical size at 96 DPI |
| `center` | `true` | Center on screen |
| `resizable` | `true` | Resize handles |
| `always_on_top` | `false` | Topmost window |
| `show` | `true` | Show when ready |
| `devtools` | `false` | F12 DevTools |
| `frame` | `true` | Native titlebar |
| `shadow` | `true` | DWM shadow |
| `url` | `https://assets.kutie/index.html` | Initial navigation |
| `parent_id` | `0` | Parent window id |
| `modal` | `false` | Modal child (requires `parent_id`) |
