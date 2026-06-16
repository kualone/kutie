# API Reference

## Runtime

```cpp
kutie::Runtime::Config cfg;
cfg.shell.title = "App";
cfg.shell.width = 1024;
cfg.shell.height = 768;
cfg.shell.center = true;
cfg.shell.resizable = true;
cfg.shell.always_on_top = false;
cfg.shell.devtools = true;
cfg.shell.decorations = true;
cfg.shell.shadow = true;
cfg.shell.background = kutie::Color{18, 16, 32};
cfg.shell.entry_url = "https://assets.kutie/index.html";

kutie::Runtime app(cfg);
app.OnReady([](kutie::Runtime& rt) { /* before message loop */ });
int code = app.Run();

app.ipc();    // IpcHub&
app.assets(); // AssetBundle&
app.shell();  // IShell&
```

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

## IShell

| Method | Description |
|---|---|
| `Run()` | Message loop |
| `Close()` | Request close |
| `ExecuteScript(js)` | Run JS in WebView |
| `SetTitle` / `SetSize` / `SetPosition` | Geometry |
| `SetAlwaysOnTop` / `SetResizable` | Window flags |
| `SetDecorations(bool)` | Native vs frameless |
| `SetBackground(Color)` | WebView2 default color, frameless gutter, Win11 DWM border |
| `Minimize` / `Maximize` / `Restore` / `ToggleMaximize` | State |
| `StartDrag()` | Native titlebar drag |
| `ToggleDevTools()` | DevTools window |
| `OnClose` / `OnResize` / … | Lifecycle callbacks |
| `NativeHandle()` | `HWND` on Windows |

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
kutie.off('event.name', handler);
await kutie.window.startDrag();
```

## ShellConfig Fields

| Field | Default | Description |
|---|---|---|
| `title` | `"Kutie App"` | Window title |
| `width` / `height` | 1024 × 768 | Logical size at 96 DPI |
| `center` | `true` | Center on screen |
| `resizable` | `true` | Resize handles |
| `always_on_top` | `false` | Topmost window |
| `devtools` | `false` | F12 DevTools |
| `decorations` | `true` | Native titlebar |
| `shadow` | `true` | DWM shadow when frameless |
| `background` | dark purple | Launch background |
| `entry_url` | `https://assets.kutie/index.html` | Initial navigation |
