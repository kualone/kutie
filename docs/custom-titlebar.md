# Custom Titlebar

Kutie supports frameless windows with a frontend-drawn titlebar, similar in UX to [Tauri window customization](https://v2.tauri.app/learn/window-customization/) but with Kutie-specific APIs.

## Enable Frameless Mode

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

## HTML Template

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

## Drag Region

| Approach | Usage |
|---|---|
| `data-kutie-drag-region` | Auto-bound on DOM ready (recommended) |
| `data-kutie-drag-region="no-maximize"` | Drag only; disable double-click maximize |
| `data-kutie-no-drag` | Opt out of drag on a child inside a drag region |
| `kutie.window.startDrag()` | Manual drag from custom `mousedown` handler |

Clicks on `button`, `a`, `input`, `select`, and `textarea` inside a drag region do **not** start a drag. Double-click on the drag region (outside interactive children) toggles maximize. Drag starts once the pointer moves about 4 px so double-click is not swallowed.

Frameless mode keeps an 8 px (DPI-scaled) resize border on the host window. WebView2 is inset by the same amount so edge drags reach native hit-testing. The gutter is painted with `shell.background`.

**Windows 11:** `DWMWA_BORDER_COLOR` matches `shell.background`, rounded corners, and `DWMNCRP_ENABLED` for shadow (no `DwmExtendFrameIntoClientArea` inset).

**Windows 10/11 frameless:** omits `WS_THICKFRAME` — the grey sizing band comes from that flag plus DWM. Resize uses `WM_NCHITTEST` on the host gutter instead. Maximize respects the monitor work area (`WM_GETMINMAXINFO`) so the taskbar stays visible.

## CSS Tips

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
  padding-top: 36px; /* leave room for fixed titlebar */
}

.titlebar button {
  -webkit-app-region: no-drag; /* not reliable alone; buttons use click handlers */
}
```

## Window Controls

Use built-in IPC handlers:

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

Run `build\sample.exe` and click **Custom titlebar** to switch modes at runtime.

See also [windows-backend.md](windows-backend.md).
