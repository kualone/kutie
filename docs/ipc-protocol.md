# IPC Protocol

Kutie uses a versioned JSON envelope over WebView2 `postMessage`.

## Envelope Format

All messages include `"v": 1`.

### JS → C++ (call)

```json
{
  "v": 1,
  "type": "call",
  "id": 42,
  "name": "greet",
  "payload": { "name": "World" }
}
```

### C++ → JS (reply)

Success:

```json
{
  "v": 1,
  "type": "reply",
  "id": 42,
  "ok": true,
  "data": { "message": "Hello, World" }
}
```

Failure:

```json
{
  "v": 1,
  "type": "reply",
  "id": 42,
  "ok": false,
  "error": "Unknown handler: missing"
}
```

### C++ → JS (event)

```json
{
  "v": 1,
  "type": "event",
  "name": "heartbeat",
  "data": { "tick": 1 }
}
```

## Client API

Injected as `window.kutie`:

| Method | Description |
|---|---|
| `kutie.call(name, payload?)` | Returns `Promise<data>` |
| `kutie.on(event, fn)` | Subscribe; returns unsubscribe function |
| `kutie.off(event, fn)` | Unsubscribe |
| `kutie.window.startDrag()` | Native window drag (custom titlebar) |

## Built-in Handlers

Registered by `Runtime`:

| Name | Payload | Action |
|---|---|---|
| `shell.minimize` | — | Minimize window |
| `shell.maximize` | — | Maximize window |
| `shell.restore` | — | Restore window |
| `shell.toggle_maximize` | — | Toggle maximize |
| `shell.close` | — | Close window |
| `shell.start_drag` | — | Start native drag (`WM_SYSCOMMAND SC_DRAGMOVE`) |
| `shell.set_title` | `{ "title": "..." }` | Set title |
| `shell.set_decorations` | `{ "decorations": bool }` | Toggle native titlebar |
| `shell.set_background` | `{ "r", "g", "b" }` or `{ "hex": "#rrggbb" }` | Update window/WebView2 background and frameless DWM border |

## C++ Registration

```cpp
app.ipc().RegisterHandler("my.action", [](const nlohmann::json& payload) {
    return nlohmann::json{{"ok", true}};
});

app.ipc().Broadcast("my.event", {{"value", 1}});
```

## Versioning

Increment `v` when breaking envelope fields. Client bootstrap rejects unknown major versions.

## Security Notes

- Event delivery uses JSON serialization (no string concatenation into JS)
- Only registered handler names are callable
- Kutie has no permission/allowlist layer (unlike Tauri); register handlers deliberately
