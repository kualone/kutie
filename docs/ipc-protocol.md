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
| `kutie.BrowserWindow.getCurrent()` | API object for the calling window |
| `kutie.BrowserWindow.create(options)` | Create a new window |
| `kutie.BrowserWindow.getAll()` | All window API objects |
| `kutie.BrowserWindow.getFocused()` | Focused window or `null` |

Window instance methods (`close`, `minimize`, `setFrame`, `startDrag`, …) call built-in `window.*` handlers with `{ id }`.

## Built-in Handlers

Registered by `Runtime`:

| Name | Payload | Action |
|---|---|---|
| `window.create` | `BrowserWindowOptions` fields | Create window → `{ id }` |
| `window.close` | `{ "id"?: number }` | Close window (default: caller) |
| `window.getAll` | — | `{ ids: [...] }` |
| `window.getFocused` | — | `{ id: number }` (`0` if none) |
| `window.show` / `window.hide` / `window.focus` | `{ "id"?: number }` | Visibility / focus |
| `window.minimize` / `maximize` / `restore` / `toggleMaximize` | `{ "id"?: number }` | Window state |
| `window.setTitle` | `{ "id"?, "title": "..." }` | Set title |
| `window.setFrame` | `{ "id"?, "frame": bool }` | Native vs partial decoration |
| `window.startDrag` | `{ "id"?: number }` | Native drag |

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
