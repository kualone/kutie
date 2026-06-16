# Windows 后端

Kutie 一期通过 Win32 + WebView2 实现 `IShell`（`WinShell`）。

## 初始化顺序

1. `EnsurePerMonitorDpiAwareness()` — Per-Monitor V2
2. 创建 Win32 窗口（导航完成前隐藏）
3. `CreateCoreWebView2EnvironmentWithOptions`
4. `CreateCoreWebView2Controller`
5. 设置 WebView2 默认背景色
6. 映射虚拟主机 `assets.kutie` 并安装 `WebResourceRequested` 过滤器
7. 注入 IPC 引导脚本（`AddScriptToExecuteOnDocumentCreated`）
8. 导航至 `entry_url`
9. 首次导航完成后显示窗口

## 资源拦截

- 过滤器：`https://assets.kutie/*`
- 根据 `AssetBundle` 解析路径
- 以内存流响应，附带 `Content-Type`、CORS、`Cache-Control: no-cache`
- 开发回退：WebView2 文件夹映射

## 无边框窗口（partial decoration）

当 `decorations = false` 时（Saucer 风格 partial decoration）：

- 样式：`WS_OVERLAPPED | WS_MINIMIZEBOX | WS_MAXIMIZEBOX`，可缩放时含 `WS_THICKFRAME`（无 `WS_CAPTION`）
- `WM_NCCALCSIZE`：按 `WINDOWINFO` 边框内缩客户区，使原生缩放环位于 WebView2 之外
- `DwmExtendFrameIntoClientArea`：顶框延伸 2 px，与自定义标题栏融合
- 拖拽：`shell.start_drag` → `WM_SYSCOMMAND SC_DRAGMOVE`
- 缩放：OS 原生边框拖拽（无需 IPC）
- **Win11 阴影：** `shadow = true` 时 `DWMNCRP_ENABLED` + `DWMWCP_ROUND`
- **Win11 边框：** `DWMWA_BORDER_COLOR` 与 `shell.background` 一致
- **Win10：** 通过 `WS_THICKFRAME` 保留系统阴影；顶部边框可能无法原生拖拽缩放

请勿单独依赖 CSS `-webkit-app-region: drag`，请使用 `data-kutie-drag-region` 或 `kutie.window.startDrag()` IPC。

## DPI

- Manifest 声明 Per-Monitor V2
- 运行时调用 `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)`（若可用）
- `WM_DPICHANGED` 时调整窗口与 WebView 边界

## 依赖

- WebView2 SDK，vcpkg triplet **`x64-windows-static`**
- 静态链接：`WebView2LoaderStatic`（配置阶段强制检查）
- 静态 MSVC 运行时（`/MT`）— 无需 VC++ 可再发行组件
- 系统：Windows 10/11 且已安装 **WebView2 Runtime**（Evergreen；不会打进 EXE）

## 单 EXE 部署

Release 构建产物为单个可执行文件，包含：

- Kutie 与 nlohmann-json 编译进 EXE
- `kutie_embed_frontend()` 嵌入的前端资源
- 静态链接的 `WebView2LoaderStatic`（无需 `WebView2Loader.dll`）

基于 Chromium 的 WebView2 Runtime 仍是系统级组件。多数 Windows 11 已预装；Windows 10 可能需要一次性安装。

构建后可用以下命令验证链接：

```powershell
dumpbin /dependents build\sample\sample.exe
```

输出中**不应**出现 `WebView2Loader.dll`、`MSVCP140.dll` 或 `VCRUNTIME140.dll`。

## 用户数据

WebView2 配置文件目录：`%TEMP%\kutie_webview2_<pid>`。
