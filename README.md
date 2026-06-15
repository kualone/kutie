# Kutie

A lightweight C++ desktop framework powered by WebView2. Build modern desktop apps with web frontends and native C++ backends — single-file deployment with embedded assets.

**Phase 1:** Windows only. macOS and Linux are planned.

## Features

- **Single EXE deployment** — WebView2Loader statically linked; frontend assets embedded in the executable
- **In-memory asset serving** — HTML/CSS/JS served from memory via `AssetBundle`; no temp files
- **Bidirectional IPC** — Promise-style `kutie.call()` and event `kutie.on()` / C++ `broadcast()`
- **Custom titlebar** — Frameless window mode with `data-kutie-drag-region` and `kutie.window.startDrag()`
- **Window API** — title, size, position, minimize/maximize/restore, always-on-top, resizable, icon
- **Lifecycle callbacks** — close (veto), resize, minimize, maximize, focus
- **File dialogs & clipboard** — open/save/folder picker, read/write clipboard
- **DevTools** — F12 toggle when `config.shell.devtools = true`
- **SPA fallback** — unknown routes serve `/index.html`
- **Per-Monitor DPI V2** — crisp rendering on high-DPI displays
- **Dark launch** — window and WebView2 background match your theme
- **Dev/Prod modes** — embedded assets in release; filesystem fallback in development

## Quick Start

### Prerequisites

- Visual Studio 2019+ (Desktop development with C++)
- [vcpkg](https://vcpkg.io) with `webview2:x64-windows`
- Python 3.7+
- PowerShell 5.1+

### Setup (first time)

```powershell
.\build.ps1 -SetupDeps
```

### Build & Run

```powershell
.\build.ps1
.\build\sample.exe
```

Output: `build\sample.exe`

## Project Structure

```
kutie/
├── include/kutie/          # Public C++ API
├── src/                    # Runtime, IPC, platform backends
├── sample/                 # Demo app
├── tools/embed_assets.py   # Frontend → .rc embedder
├── docs/                   # Detailed documentation
├── CHANGELOG.md
└── build.ps1
```

## Minimal Example

### C++

```cpp
#include "kutie/runtime.hpp"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    kutie::Runtime::Config cfg;
    cfg.shell.title = "My App";
    cfg.shell.devtools = true;

    kutie::Runtime app(cfg);
    app.ipc().RegisterHandler("greet", [](const nlohmann::json& payload) {
        std::string name = payload.value("name", "World");
        return nlohmann::json{{"message", "Hello, " + name}};
    });
    return app.Run();
}
```

### JavaScript

```javascript
const result = await kutie.call('greet', { name: 'Kutie' });
kutie.on('heartbeat', (data) => console.log(data));
```

### Custom Titlebar

```cpp
cfg.shell.decorations = false;  // frameless window
```

```html
<div class="titlebar" data-kutie-drag-region>
  <button onclick="kutie.call('shell.close')">×</button>
</div>
```

See [docs/custom-titlebar.md](docs/custom-titlebar.md).

## Architecture

```
Runtime → IpcHub + AssetBundle + IShell (WinShell / WebView2)
```

Details: [docs/architecture.md](docs/architecture.md)

## Documentation

| Document | Description |
|---|---|
| [architecture.md](docs/architecture.md) | Module design |
| [ipc-protocol.md](docs/ipc-protocol.md) | IPC envelope format |
| [asset-bundling.md](docs/asset-bundling.md) | Dev/prod asset loading |
| [windows-backend.md](docs/windows-backend.md) | WebView2 backend |
| [custom-titlebar.md](docs/custom-titlebar.md) | Custom titlebar guide |
| [api-reference.md](docs/api-reference.md) | Full API reference |
| [roadmap.md](docs/roadmap.md) | Cross-platform roadmap |
| [CHANGELOG.md](CHANGELOG.md) | Version history |

## Comparison

| | Kutie | Electron | Tauri | WPF |
|---|---|---|---|---|
| Backend | C++ | Node.js | Rust | C# |
| Renderer | WebView2 | Chromium | System WebView | DirectX |
| Binary size | ~1–10 MB | 100+ MB | 3–10 MB | .NET required |
| Phase 1 platform | Windows | All | All | Windows |

## License

MIT — see [LICENSE](LICENSE).
