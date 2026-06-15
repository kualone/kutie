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
- Shadow: `DWMWA_NCRENDERING_POLICY` enabled when `shadow = true`

Do not rely on CSS `-webkit-app-region: drag` — use native drag via IPC.

## DPI

- Manifest declares Per-Monitor V2
- Runtime calls `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)` when available
- `WM_DPICHANGED` resizes window and WebView bounds

## Dependencies

- WebView2 SDK via vcpkg (`webview2:x64-windows`)
- Static link: `WebView2LoaderStatic`
- System: Windows 10/11 with WebView2 Runtime

## User Data

WebView2 profile stored under `%TEMP%\kutie_webview2_<pid>`.
