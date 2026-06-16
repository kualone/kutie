# Roadmap

## Phase 1 — Windows (current)

- [x] WebView2 backend (`WinBrowserWindow`)
- [x] IPC, asset embedding, sample app
- [x] Custom titlebar
- [x] Dialogs, clipboard, DPI, DevTools
- [x] Multi-window + modal (`BrowserWindow`)

## Phase 2 — macOS

- [ ] `MacShell` with Cocoa + WKWebView
- [ ] Custom URL scheme handler for `https://assets.kutie/`
- [ ] `TitleBarMode::Overlay` — macOS titlebar overlay (`hiddenTitle` + traffic light inset)
- [ ] `data-kutie-drag-region` via `WKScriptMessageHandler`
- [ ] `.app` bundle packaging

## Phase 3 — Linux

- [ ] `GtkShell` with GTK3 + WebKitGTK 4.1
- [ ] `webkit_web_context_register_uri_scheme` for assets
- [ ] IPC via `webkit_user_content_manager_register_script_message_handler`
- [ ] AppImage / deb packaging

## Cross-cutting (future)

- [ ] Portable `embedded_assets.cpp` generator (replace Windows-only `.rc` in shared builds)
- [ ] Runtime navigate / `loadURL` IPC
- [ ] System tray / notifications
- [ ] Auto-updater hooks

Contributions welcome once Phase 1 stabilizes.
