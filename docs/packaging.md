# Packaging & Consumption

How to integrate Kutie into another C++ project via CMake.

## Overview

Kutie installs as a CMake package with:

| Artifact | Purpose |
|---|---|
| `Kutie::kutie` | Static library target |
| `kutie_embed_frontend()` | Embed your `frontend/` into an executable |
| `kutie_apply_windows_manifest()` | Per-Monitor V2 DPI manifest (MSVC) |
| `share/kutie/tools/embed_assets.py` | Asset code generator |

Frontend embedding uses **registration callbacks**: generated code registers a loader with `AssetBundle::RegisterEmbeddedLoader()` at static initialization. The Kutie library itself contains no app-specific assets.

## Prerequisites

- Windows (Phase 1)
- Visual Studio 2019+ with C++ desktop workload
- CMake 3.20+
- vcpkg with `nlohmann-json` and `webview2`
- Python 3.7+ (for `kutie_embed_frontend`)

## Option 1: `find_package(Kutie)` (installed package)

### Install Kutie

```powershell
cmake -B build -S . -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DKUTIE_BUILD_SAMPLES=OFF
cmake --build build
cmake --install build --prefix install
```

### Consumer `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyApp LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "C:/path/to/kutie/install")

find_package(Kutie CONFIG REQUIRED)

add_executable(myapp WIN32 src/main.cpp)
target_link_libraries(myapp PRIVATE Kutie::kutie)

kutie_embed_frontend(myapp "${CMAKE_SOURCE_DIR}/frontend")
kutie_apply_windows_manifest(myapp)
```

### Consumer `vcpkg.json`

```json
{
  "dependencies": [
    "nlohmann-json",
    "webview2"
  ]
}
```

Configure with the vcpkg toolchain file so `find_dependency` inside `KutieConfig.cmake` resolves WebView2 and nlohmann-json.

## Option 2: Git submodule + `add_subdirectory`

```powershell
git submodule add https://github.com/<you>/kutie.git third_party/kutie
```

```cmake
set(KUTIE_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/kutie)

add_executable(myapp WIN32 src/main.cpp)
target_link_libraries(myapp PRIVATE Kutie::kutie)
kutie_embed_frontend(myapp "${CMAKE_SOURCE_DIR}/frontend")
kutie_apply_windows_manifest(myapp)
```

When using `add_subdirectory`, CMake helpers are available automatically; `_KUTIE_PACKAGE_ROOT` points at the Kutie source tree.

## Option 3: vcpkg overlay port

This repository includes `ports/kutie/` for local overlay usage.

**`vcpkg-configuration.json`** (consumer project):

```json
{
  "overlay-ports": ["path/to/kutie/ports"]
}
```

**`vcpkg.json`**:

```json
{
  "dependencies": ["kutie"]
}
```

```powershell
vcpkg install
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

## Application entry point

```cpp
#include "kutie/runtime.hpp"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    kutie::Runtime::Config cfg;
    cfg.shell.title = "My App";
    // Optional dev disk fallback override:
    // cfg.dev_frontend_path = "C:/dev/myapp/frontend";

    kutie::Runtime app(cfg);
    app.ipc().RegisterHandler("hello", [](const nlohmann::json& args) {
        return nlohmann::json{{"message", "Hello"}};
    });
    return app.Run();
}
```

## Development without embedding

Omit `kutie_embed_frontend()` and place a `frontend/` folder next to your executable, or set `cfg.dev_frontend_path`. Runtime falls back to `LoadFromDisk()` automatically.

## CMake options (Kutie project)

| Option | Default (standalone) | Description |
|---|---|---|
| `KUTIE_BUILD_SAMPLES` | ON | Build `sample.exe` |
| `KUTIE_INSTALL` | ON | Generate install rules |

## Edge cases

- **Static init order**: embedded loaders register before `main()`; safe because registration only appends to a static vector.
- **Multiple embed calls**: one `kutie_embed_frontend()` per executable target.
- **Missing Python**: `kutie_embed_frontend()` requires Python 3 at configure/build time.
