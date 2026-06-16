# IPC 协议

Kutie 通过 WebView2 `postMessage` 传输带版本号的 JSON 信封。

## 信封格式

所有消息包含 `"v": 1`。

### JS → C++（调用）

```json
{
  "v": 1,
  "type": "call",
  "id": 42,
  "name": "greet",
  "payload": { "name": "World" }
}
```

### C++ → JS（回复）

成功：

```json
{
  "v": 1,
  "type": "reply",
  "id": 42,
  "ok": true,
  "data": { "message": "Hello, World" }
}
```

失败：

```json
{
  "v": 1,
  "type": "reply",
  "id": 42,
  "ok": false,
  "error": "Unknown handler: missing"
}
```

### C++ → JS（事件）

```json
{
  "v": 1,
  "type": "event",
  "name": "heartbeat",
  "data": { "tick": 1 }
}
```

## 客户端 API

注入为 `window.kutie`：

| 方法 | 说明 |
|---|---|
| `kutie.call(name, payload?)` | 返回 `Promise<data>` |
| `kutie.on(event, fn)` | 订阅；返回取消订阅函数 |
| `kutie.off(event, fn)` | 取消订阅 |
| `kutie.BrowserWindow.getCurrent()` | 当前窗口 API 对象 |
| `kutie.BrowserWindow.create(options)` | 创建新窗口 |
| `kutie.BrowserWindow.getAll()` | 所有窗口 API 对象 |
| `kutie.BrowserWindow.getFocused()` | 聚焦窗口或 `null` |

窗口实例方法（`close`、`minimize`、`setFrame`、`startDrag` 等）会调用内置 `window.*` handler，并附带 `{ id }`。

## 内置 Handler

由 `Runtime` 注册：

| 名称 | Payload | 作用 |
|---|---|---|
| `window.create` | `BrowserWindowOptions` 字段 | 创建窗口 → `{ id }` |
| `window.close` | `{ "id"?: number }` | 关闭窗口（默认当前） |
| `window.getAll` | — | `{ ids: [...] }` |
| `window.getFocused` | — | `{ id: number }`（无则 `0`） |
| `window.show` / `window.hide` / `window.focus` | `{ "id"?: number }` | 显示 / 隐藏 / 聚焦 |
| `window.minimize` / `maximize` / `restore` / `toggleMaximize` | `{ "id"?: number }` | 窗口状态 |
| `window.setTitle` | `{ "id"?, "title": "..." }` | 设置标题 |
| `window.setFrame` | `{ "id"?, "frame": bool }` | 原生 / partial 装饰 |
| `window.startDrag` | `{ "id"?: number }` | 原生拖拽 |

## C++ 注册

```cpp
app.ipc().RegisterHandler("my.action", [](const nlohmann::json& payload) {
    return nlohmann::json{{"ok", true}};
});

app.ipc().Broadcast("my.event", {{"value", 1}});
```

## 版本控制

破坏信封字段时递增 `v`。客户端 bootstrap 会拒绝未知主版本。

## 安全说明

- 事件投递使用 JSON 序列化（不会拼接进 JS 字符串）
- 仅已注册 handler 可被调用
- Kutie 无 Tauri 式权限白名单；请谨慎注册 handler
