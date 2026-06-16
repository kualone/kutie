# 自定义标题栏

Kutie 支持无边框窗口 + 前端绘制标题栏，交互类似 [Tauri 窗口定制](https://v2.tauri.app/learn/window-customization/)，API 为 Kutie 自有。

## 启用无边框模式

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

无边框模式下保留 8 px（按 DPI 缩放）的原生缩放边框；其余区域为 WebView 客户区。

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

.titlebar button {
  -webkit-app-region: no-drag; /* 单独设置不可靠；按钮应使用 click + IPC */
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

运行 `build\sample.exe`，点击 **Custom titlebar** 可在运行时切换模式。

另见 [windows-backend.zh.md](windows-backend.zh.md)。
