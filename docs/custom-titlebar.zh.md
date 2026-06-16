# 自定义标题栏

Kutie 采用 **partial decoration**（部分装饰）模型实现自定义标题栏，与 [Saucer `decoration::partial`](https://saucer.github.io/window/decorations/) 思路一致：标题栏由前端 HTML 绘制，缩放边框、Aero Snap 与阴影由系统原生提供。

| | 自定义标题栏（`decorations = false`） | 原生（`decorations = true`） |
|---|---|---|
| 标题栏 | 前端 HTML | 系统 |
| 缩放边框 | OS 原生（`WS_THICKFRAME`） | OS 原生 |
| Aero Snap | 支持 | 支持 |
| 窗口阴影 | 支持（`shadow = true` 时） | 支持 |

## 启用 partial decoration

### C++（启动时）

```cpp
kutie::Runtime::Config cfg;
cfg.shell.decorations = false;
cfg.shell.shadow = true;
```

### 运行时切换

```javascript
await kutie.call('shell.set_decorations', { decorations: false });
```

## HTML 模板

```html
<div class="titlebar" data-kutie-drag-region>
  <span class="title">My App</span>
  <div class="controls">
    <button onclick="kutie.call('shell.minimize')">—</button>
    <button onclick="kutie.call('shell.toggle_maximize')">□</button>
    <button onclick="kutie.call('shell.close')">×</button>
  </div>
</div>
```

## 拖拽区域

| 方式 | 说明 |
|---|---|
| `data-kutie-drag-region` | DOM 就绪后自动绑定（推荐） |
| `data-kutie-drag-region="no-maximize"` | 仅拖拽，禁用双击最大化 |
| `data-kutie-no-drag` | 拖拽区域内的子元素不参与拖拽 |
| `kutie.window.startDrag()` | 在自定义 `mousedown` 中手动拖拽 |

拖拽区域内对 `button`、`a`、`input`、`select`、`textarea` 的点击**不会**触发拖拽。在拖拽区域（非交互子元素）上双击可切换最大化。指针移动约 4 px 后才开始拖拽，避免双击被吞掉。

拖拽通过顶层 HWND 的 `WM_SYSCOMMAND SC_DRAGMOVE`（Qt 同款）实现，请勿单独依赖 CSS `-webkit-app-region`。

## 缩放边框

**无需前端代码。** 当 `decorations = false` 且 `resizable = true` 时，Kutie 保留 `WS_THICKFRAME`，并在 `WM_NCCALCSIZE` 中将原生缩放环置于 WebView2 客户区之外。直接拖拽窗口边缘或角落即可缩放。

**Win10 说明：** 顶部边框可能无法原生拖拽缩放。左、右、底边与四角正常。

请将 `shell.background` 与页面 `--bg` 同步，避免 Win11 圆角处出现色差。

## 阴影与 DWM

`shadow = true`（默认）时：

- **Windows 11：** `DWMNCRP_ENABLED` + `DWMWCP_ROUND`；`DWMWA_BORDER_COLOR` 与 `shell.background` 一致。
- **Windows 10：** partial decoration + `WS_THICKFRAME` 保留系统阴影；`DWMWA_BORDER_COLOR` 与 `shell.background` 一致。

`DwmExtendFrameIntoClientArea` 将顶框延伸 2 px，使 DWM 与自定义标题栏融合。运行时切换自定义标题栏无需重建窗口。

最大化时通过 `WM_GETMINMAXINFO` 与工作区 snap 限制在工作区内，不遮挡任务栏。

## CSS 提示

```css
.titlebar {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 36px;
  user-select: none;
  -webkit-user-select: none;
}

body {
  padding-top: 36px; /* 为固定标题栏留空 */
}
```

## 窗口控制

内置 IPC：

- `shell.minimize`
- `shell.maximize` / `shell.restore`
- `shell.toggle_maximize`
- `shell.close`

## 与 Tauri 的差异

| Tauri | Kutie |
|---|---|
| `decorations: false` | `cfg.shell.decorations = false` |
| `data-tauri-drag-region` | `data-kutie-drag-region` |
| `window.startDragging()` | `kutie.window.startDrag()` |
| 权限 / allowlist | 无 allowlist；在 C++ 注册 handler |

## 示例

运行 `build\sample.exe`，点击 **Custom titlebar** 可在运行时切换模式。边框拖拽缩放由 OS 处理，请手动拖拽窗口边缘验证。

另见 [windows-backend.zh.md](windows-backend.zh.md)。
