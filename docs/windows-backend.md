# Windows Backend

Kutie Phase 1 implements `IShell` as `WinShell` using Win32 and WebView2.

## Initialization Sequence

1. `EnsurePerMonitorDpiAwareness()` — Per-Monitor V2
2. Create Win32 window (hidden until navigation completes)
3. `CreateCoreWebView2EnvironmentWithOptions`
4. `CreateCoreWebView2Controller`
5. Set WebView2 default background color
6. Map virtual host `assets.kutie` + install `WebResourceRequested` filter
7. Inject IPC bootstrap script (`AddScriptToExecuteOnDocumentCreated`)
8. Navigate to `entry_url`
9. Show window after first navigation completes

## Resource Interception

- Filter: `https://assets.kutie/*`
- Resolve path against `AssetBundle`
- Respond with in-memory stream, `Content-Type`, CORS, `Cache-Control: no-cache`
- Fallback: WebView2 folder mapping for dev scenarios

## Frameless Window

When `decorations = false`:

- Style: `WS_OVERLAPPED | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX` (no `WS_CAPTION`)
- Drag: `ReleaseCapture()` + `SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0)`
- **Win11 shadow:** `DWMNCRP_ENABLED` + `DWMWCP_ROUND` when `shadow = true` (margins stay zero)
- **Win11 border:** `DWMWA_BORDER_COLOR` matches `shell.background`
- **Frameless (all):** no `WS_THICKFRAME`; grey sizing bands avoided, resize via host `WM_NCHITTEST`
- **Win10 DWM:** `DWMNCRP_DISABLED`
- **Resize (both):** host `WM_NCHITTEST` on an 8 px gutter; WebView2 inset to expose the gutter

Do not rely on CSS `-webkit-app-region: drag` — use native drag via IPC.

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
