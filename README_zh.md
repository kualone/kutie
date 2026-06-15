# Kutie

基于 WebView2 的轻量级 C++ 桌面应用框架。使用 Web 前端 + C++ 后端构建现代桌面应用，支持单 EXE 部署与资源内存加载。

**一期范围：** 仅 Windows。macOS / Linux 见 [路线图](docs/roadmap.md)。

## 特性

- **单 EXE 部署** — WebView2Loader 静态链接，前端资源嵌入可执行文件
- **内存资源服务** — `AssetBundle` 从内存提供 HTML/CSS/JS，不写入临时文件
- **双向 IPC** — `kutie.call()` Promise 风格调用，C++ `broadcast()` 推送事件
- **自定义标题栏** — `decorations=false` + `data-kutie-drag-region` 拖拽区域
- **窗口 API** — 标题、尺寸、位置、最小化/最大化、置顶、可调整大小等
- **生命周期回调** — 关闭（可否决）、resize、minimize、maximize、focus
- **文件对话框与剪贴板**
- **DevTools** — F12 切换（需 `devtools=true`）
- **SPA 回退** — 非静态路径返回 `/index.html`
- **Per-Monitor V2 高 DPI**
- **深色启动** — 窗口与 WebView2 背景色匹配主题
- **开发/生产双模式** — 生产嵌入资源，开发自动读 `sample/frontend`

## 快速开始

### 环境要求

- Visual Studio 2019+（C++ 桌面开发）
- vcpkg + `webview2:x64-windows`
- Python 3.7+
- PowerShell 5.1+

### 安装依赖

```powershell
.\build.ps1 -SetupDeps
```

### 构建与运行

```powershell
.\build.ps1
.\build\sample.exe
```

## 最小示例

### C++

```cpp
#include "kutie/runtime.hpp"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    kutie::Runtime::Config cfg;
    cfg.shell.title = "我的应用";
    kutie::Runtime app(cfg);

    app.ipc().RegisterHandler("greet", [](const nlohmann::json& args) {
        return nlohmann::json{{"message", "你好, " + args.value("name", "World")}};
    });
    return app.Run();
}
```

### JavaScript

```javascript
const res = await kutie.call('greet', { name: 'Kutie' });
kutie.on('heartbeat', (data) => console.log(data));
```

## 文档

- [架构](docs/architecture.md)
- [IPC 协议](docs/ipc-protocol.md)
- [资源打包](docs/asset-bundling.md)
- [Windows 后端](docs/windows-backend.md)
- [自定义标题栏](docs/custom-titlebar.md)
- [API 参考](docs/api-reference.md)
- [打包与引用](docs/packaging.zh.md)
- [路线图](docs/roadmap.md)
- [更新日志](CHANGELOG.md)

## 许可证

MIT
