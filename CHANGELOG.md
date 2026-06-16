# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Changed

- **Breaking:** Replaced `IShell` / `ShellConfig` with `BrowserWindow` / `BrowserWindowOptions` (Electron-style API)
- **Breaking:** Removed `Runtime::shell()` and all `shell.*` IPC handlers; use `window.*` and `kutie.BrowserWindow`
- **Breaking:** Renamed `decorations` → `frame`, `entry_url` → `url` in window options
- `Runtime::Run()` now owns the message loop; windows share a single WebView2 environment

### Added

- Multi-window support: `BrowserWindow::Create()`, `GetAll()`, `GetFocused()`, `FromId()`
- Modal child windows (`parent_id` + `modal: true`) on Windows
- JS API: `kutie.BrowserWindow.create()` / `getCurrent()` / instance methods
- Smoke tests: `kutie_smoke_browser_window`, `tools/smoke/multi-window.ps1`
- Docs: `docs/features/browser-window.md` (+ Chinese)

## [0.1.0] - 2026-06-15

### Added

- Initial Windows release with WebView2 backend (`WinShell`)
- `Runtime`, `IpcHub`, `AssetBundle`, `PlatformServices` public API
- IPC envelope protocol (`call` / `reply` / `event`) with `window.kutie` client
- Embedded frontend assets via `tools/embed_assets.py` and `KUTIE_ASSET` resources
- Dev/prod asset loading (embedded → filesystem fallback)
- SPA route fallback to `/index.html`
- Window operations: title, size, position, min/max/restore, always-on-top, resizable, icon
- Custom titlebar: `decorations`, `SetDecorations()`, `StartDrag()`, `data-kutie-drag-region`
- Lifecycle callbacks: close (veto), resize, minimize, maximize, focus
- File dialogs (open/save/folder), clipboard, message boxes
- DevTools toggle (F12)
- Per-Monitor V2 DPI awareness
- Dark launch background color
- Sample demo with native/custom titlebar switch
- Documentation under `docs/` and bilingual README

[0.1.0]: https://github.com/kunlonly/kutie/releases/tag/v0.1.0
