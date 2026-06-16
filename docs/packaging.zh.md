# 打包与引用

如何在其他 C++ 项目中通过 CMake 集成 Kutie。

## 概览

Kutie 以 CMake 包形式提供：

| 产物 | 用途 |
|---|---|
| `Kutie::kutie` | 静态库目标 |
| `kutie_embed_frontend()` | 将 `frontend/` 嵌入可执行文件 |
| `kutie_apply_windows_manifest()` | Per-Monitor V2 DPI manifest（MSVC） |
| `share/kutie/tools/embed_assets.py` | 资源代码生成脚本 |

前端嵌入采用**注册回调**：生成代码在静态初始化阶段调用 `AssetBundle::RegisterEmbeddedLoader()`。Kutie 库本身**不包含**任何应用专属资源。

## 环境要求

- Windows（一期）
- Visual Studio 2019+（C++ 桌面开发）
- CMake 3.20+
- vcpkg：`nlohmann-json`、`webview2`，triplet **`x64-windows-static`**
- Python 3.7+（`kutie_embed_frontend` 需要）

`build.ps1` 与 Kutie CMake 默认使用 `x64-windows-static`，以静态链接 `WebView2LoaderStatic` 与 `/MT`，无需额外 DLL。手动配置时请传入相同 triplet：

```powershell
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

## 方式一：`find_package(Kutie)`（已安装包）

### 安装 Kutie

```powershell
cmake -B build -S . -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DKUTIE_BUILD_SAMPLES=OFF
cmake --build build
cmake --install build --prefix install
```

### 消费者 `CMakeLists.txt`

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

### 消费者 `vcpkg.json`

```json
{
  "dependencies": [
    "nlohmann-json",
    "webview2"
  ]
}
```

使用 triplet **`x64-windows-static`** 安装与配置（Kutie 默认）：

```powershell
vcpkg install --triplet=x64-windows-static
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

## 方式二：Git 子模块 + `add_subdirectory`

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

使用 `add_subdirectory` 时 CMake 辅助函数自动可用；`_KUTIE_PACKAGE_ROOT` 指向 Kutie 源码根目录。

## 方式三：vcpkg overlay port

本仓库提供 `ports/kutie/` 供本地 overlay 使用。

**`vcpkg-configuration.json`**（消费者项目）：

```json
{
  "overlay-ports": ["path/to/kutie/ports"]
}
```

**`vcpkg.json`**：

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

## 应用入口

```cpp
#include "kutie/runtime.hpp"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    kutie::Runtime::Config cfg;
    cfg.main_window.title = "我的应用";
    // 可选：开发时磁盘回退路径
    // cfg.dev_frontend_path = "C:/dev/myapp/frontend";

    kutie::Runtime app(cfg);
    app.ipc().RegisterHandler("hello", [](const nlohmann::json& args) {
        return nlohmann::json{{"message", "Hello"}};
    });
    return app.Run();
}
```

## 开发模式（不嵌入资源）

省略 `kutie_embed_frontend()`，在 exe 旁放置 `frontend/` 目录，或设置 `cfg.dev_frontend_path`。运行时会自动 `LoadFromDisk()` 回退。

## CMake 选项（Kutie 工程）

| 选项 | 默认值（独立构建） | 说明 |
|---|---|---|
| `KUTIE_BUILD_SAMPLES` | ON | 是否构建 `sample.exe` |
| `KUTIE_INSTALL` | ON | 是否生成 install 规则 |

## 终端用户分发

使用 `x64-windows-static` 构建的 **Release** 可执行文件可单独分发，无需附带：

- `WebView2Loader.dll`
- Visual C++ 可再发行组件

用户机器需已安装 **WebView2 Runtime**（多数 Windows 11 已预装）。

## 边界情况

- **静态初始化顺序**：嵌入加载器在 `main()` 前注册；仅向静态 vector 追加，安全。
- **多次嵌入**：每个可执行目标调用一次 `kutie_embed_frontend()`。
- **缺少 Python**：`kutie_embed_frontend()` 在配置/构建阶段需要 Python 3。
