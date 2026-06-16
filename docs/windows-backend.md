# Windows Backend

Kutie Phase 1 implements `IShell` as `WinShell` using Win32 and WebView2.

## Initialization Sequence

1. `EnsurePerMonitorDpiAwareness()` — Per-Monitor V2
2. Create Win32 window (hidden until navigation completes)
3. `CreateCoreWebView2EnvironmentWithOptions`
4. `CreateCoreWebView2Controller`
5. Map virtual host `assets.kutie` + install `WebResourceRequested` filter
6. Inject IPC bootstrap script (`AddScriptToExecuteOnDocumentCreated`)
7. Navigate to `entry_url`
8. Show window after first navigation completes

## Resource Interception

- Filter: `https://assets.kutie/*`
- Resolve path against `AssetBundle`
- Respond with in-memory stream, `Content-Type`, CORS, `Cache-Control: no-cache`
- Fallback: WebView2 folder mapping for dev scenarios

## Frameless Window (partial decoration)

When `decorations = false` (Saucer-style partial decoration):

- Style: `WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME` when `resizable = true` (same as Saucer `decoration::partial`)
- `WM_NCCALCSIZE`: inset left/right/bottom by `WINDOWINFO` borders; when maximized with a negative top rect, add a top offset equal to the border height
- `DwmExtendFrameIntoClientArea`: top margin 2 px for titlebar blend; resize border color uses the system default
- Drag: `shell.start_drag` → `WM_SYSCOMMAND SC_DRAGMOVE`
- Resize: OS native border drag via `WS_THICKFRAME` (no IPC required)
- **Win10:** top-edge native resize may be unavailable (Saucer documents the same trade-off)

Do not rely on CSS `-webkit-app-region: drag` alone — use `data-kutie-drag-region` or `kutie.window.startDrag()` via IPC.

## DPI

- Manifest declares Per-Monitor V2
- Runtime calls `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)` when available
- `WM_DPICHANGED` resizes window and WebView bounds

## Dependencies

- WebView2 SDK via vcpkg triplet **`x64-windows-static`**
- Static link: `WebView2LoaderStatic` (enforced at configure time)
- Static MSVC runtime (`/MT`) — no VC++ Redistributable required
- System: Windows 10/11 with **WebView2 Runtime** (Evergreen; not bundled in the EXE)

## Single-EXE deployment

Release builds produce one executable with:

- Kutie + nlohmann-json compiled in
- Frontend assets embedded via `kutie_embed_frontend()`
- `WebView2LoaderStatic` linked in (no `WebView2Loader.dll`)

The Chromium-based WebView2 Runtime remains a separate system component. Most Windows 11 machines already have it; Windows 10 may need a one-time install.

Verify linkage after build:

```powershell
dumpbin /dependents build\sample\sample.exe
```

You should **not** see `WebView2Loader.dll`, `MSVCP140.dll`, or `VCRUNTIME140.dll`.

## User Data

WebView2 profile stored under `%TEMP%\kutie_webview2_<pid>`.
