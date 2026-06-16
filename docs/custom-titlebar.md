# Custom Titlebar

Kutie supports custom titlebars using a **partial decoration** model (same idea as [Saucer `decoration::partial`](https://saucer.github.io/window/decorations/)): you draw the titlebar in HTML, while Windows keeps native resize borders, Aero Snap, and drop shadow.

| | Custom titlebar (`decorations = false`) | Native (`decorations = true`) |
|---|---|---|
| Titlebar | Frontend HTML | System |
| Resize borders | OS native (`WS_THICKFRAME`) | OS native |
| Aero Snap | Yes | Yes |
| Window shadow | Yes (when `shadow = true`) | Yes |

## Enable partial decoration

### C++ (startup)

```cpp
kutie::Runtime::Config cfg;
cfg.shell.decorations = false;
cfg.shell.shadow = true;
```

### Runtime toggle

```javascript
await kutie.call('shell.set_decorations', { decorations: false });
```

## HTML template

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

## Drag region

| Approach | Usage |
|---|---|
| `data-kutie-drag-region` | Auto-bound on DOM ready (recommended) |
| `data-kutie-drag-region="no-maximize"` | Drag only; disable double-click maximize |
| `data-kutie-no-drag` | Opt out of drag on a child inside a drag region |
| `kutie.window.startDrag()` | Manual drag from custom `mousedown` handler |

Clicks on `button`, `a`, `input`, `select`, and `textarea` inside a drag region do **not** start a drag. Double-click on the drag region (outside interactive children) toggles maximize. Drag starts once the pointer moves about 4 px so double-click is not swallowed.

Drag uses `WM_SYSCOMMAND SC_DRAGMOVE` on the top-level HWND (Qt-style), not CSS `-webkit-app-region`.

## Resize borders

**No frontend code is required.** When `decorations = false` and `resizable = true`, Kutie keeps `WS_THICKFRAME` and handles `WM_NCCALCSIZE` so the native resize ring sits outside the WebView2 client area. Drag any window edge or corner to resize.

**Win10 note:** the top border may not support native drag-resize. Left, right, bottom, and corners work normally.

Page background and theme are controlled entirely by frontend CSS.

## Shadow and DWM

`DwmExtendFrameIntoClientArea` extends the top frame by 2 px so DWM blends with your custom titlebar. Win11 resize border color follows the system default. Runtime switch to custom titlebar does not require recreating the window.

Maximize respects the monitor work area (`WM_GETMINMAXINFO`) so the taskbar stays visible.

## CSS tips

```css
.titlebar {
  height: 32px;
  display: flex;
  align-items: stretch;
  user-select: none;
  -webkit-user-select: none;
}

.titlebar-controls {
  display: flex;
  align-self: stretch;
}

.titlebar-btn {
  width: 46px;
  align-self: stretch;
  appearance: none;
}
```

Use `box-shadow` (not `border-bottom`) for the titlebar separator so button hover fills the full 32 px height.

## Window controls

Built-in IPC handlers:

- `shell.minimize`
- `shell.maximize` / `shell.restore`
- `shell.toggle_maximize`
- `shell.close`

## Differences from Tauri

| Tauri | Kutie |
|---|---|
| `decorations: false` in config | `cfg.shell.decorations = false` |
| `data-tauri-drag-region` | `data-kutie-drag-region` |
| `window.startDragging()` | `kutie.window.startDrag()` |
| Permissions / allowlist | No allowlist; handlers registered in C++ |

## Sample

Run `build\sample.exe` and click **Custom titlebar** to switch modes at runtime. Border drag-resize is handled by the OS — verify manually by dragging window edges.

See also [windows-backend.md](windows-backend.md).
