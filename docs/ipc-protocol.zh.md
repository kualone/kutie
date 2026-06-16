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
| `kutie.window.startDrag()` | 原生窗口拖拽（自定义标题栏） |

## 内置 Handler

由 `Runtime` 注册：

| 名称 | Payload | 作用 |
|---|---|---|
| `shell.minimize` | — | 最小化窗口 |
| `shell.maximize` | — | 最大化窗口 |
| `shell.restore` | — | 还原窗口 |
| `shell.toggle_maximize` | — | 切换最大化 |
| `shell.close` | — | 关闭窗口 |
| `shell.start_drag` | — | 开始原生拖拽 |
| `shell.set_title` | `{ "title": "..." }` | 设置标题 |
| `shell.set_decorations` | `{ "decorations": bool }` | 切换原生标题栏 |
| `shell.set_background` | `{ "r", "g", "b" }` 或 `{ "hex": "#rrggbb" }` | 更新窗口/WebView2 背景色及无边框 DWM 边框 |

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

- 事件投递使用 JSON 序列化（不向 JS 拼接字符串）
- 仅已注册的 handler 名称可被调用
- Kutie 无权限/白名单层（与 Tauri 不同）；请谨慎注册 handler
