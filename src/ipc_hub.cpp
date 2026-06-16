#include "kutie/ipc_hub.hpp"

#include <sstream>

namespace kutie {

namespace {

thread_local uint32_t g_dispatch_source = 0;

} // namespace

IpcHub& IpcHub::Shared() {
    static IpcHub instance;
    return instance;
}

uint32_t IpcHub::CurrentDispatchSource() {
    return g_dispatch_source;
}

void IpcHub::RegisterHandler(const std::string& name, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[name] = std::move(handler);
}

void IpcHub::UnregisterHandler(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(name);
}

void IpcHub::AttachWindow(uint32_t window_id, ScriptRunner runner) {
    std::lock_guard<std::mutex> lock(mutex_);
    window_runners_[window_id] = std::move(runner);
}

void IpcHub::DetachWindow(uint32_t window_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    window_runners_.erase(window_id);
}

std::string IpcHub::DispatchCall(
    uint32_t source_id,
    int call_id,
    const std::string& name,
    const std::string& payload_json) {
    Handler handler;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = handlers_.find(name);
        if (it == handlers_.end()) {
            nlohmann::json envelope;
            envelope["v"] = 1;
            envelope["type"] = "reply";
            envelope["id"] = call_id;
            envelope["ok"] = false;
            envelope["error"] = "Unknown handler: " + name;
            return envelope.dump();
        }
        handler = it->second;
    }

    g_dispatch_source = source_id;
    try {
        nlohmann::json payload = payload_json.empty() ? nlohmann::json::object() : nlohmann::json::parse(payload_json);
        nlohmann::json data = handler(payload);

        nlohmann::json envelope;
        envelope["v"] = 1;
        envelope["type"] = "reply";
        envelope["id"] = call_id;
        envelope["ok"] = true;
        envelope["data"] = std::move(data);
        return envelope.dump();
    } catch (const std::exception& ex) {
        nlohmann::json envelope;
        envelope["v"] = 1;
        envelope["type"] = "reply";
        envelope["id"] = call_id;
        envelope["ok"] = false;
        envelope["error"] = ex.what();
        return envelope.dump();
    }
}

void IpcHub::Broadcast(const std::string& event, const nlohmann::json& data) {
    std::unordered_map<uint32_t, ScriptRunner> runners;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        runners = window_runners_;
    }

    nlohmann::json envelope;
    envelope["v"] = 1;
    envelope["type"] = "event";
    envelope["name"] = event;
    envelope["data"] = data;

    const std::string script = "window.__kutie_deliver__(" + envelope.dump() + ");";
    for (const auto& entry : runners) {
        entry.second(script);
    }
}

std::string IpcHub::ClientBootstrapScript(uint32_t window_id) {
    std::ostringstream script;
    script << R"js(
(function(windowId) {
    function makeWindowApi(id) {
        return {
            id: id,
            close: function() { return kutie.call('window.close', { id: id }); },
            show: function() { return kutie.call('window.show', { id: id }); },
            hide: function() { return kutie.call('window.hide', { id: id }); },
            focus: function() { return kutie.call('window.focus', { id: id }); },
            minimize: function() { return kutie.call('window.minimize', { id: id }); },
            maximize: function() { return kutie.call('window.maximize', { id: id }); },
            restore: function() { return kutie.call('window.restore', { id: id }); },
            toggleMaximize: function() { return kutie.call('window.toggleMaximize', { id: id }); },
            setTitle: function(title) { return kutie.call('window.setTitle', { id: id, title: title }); },
            setFrame: function(frame) { return kutie.call('window.setFrame', { id: id, frame: frame }); },
            startDrag: function() { return kutie.call('window.startDrag', { id: id }); }
        };
    }

    const kutie = {
        _seq: 0,
        _pending: {},
        _listeners: {},

        call: function(name, payload) {
            return new Promise((resolve, reject) => {
                const id = ++this._seq;
                this._pending[id] = { resolve, reject };
                window.chrome.webview.postMessage(JSON.stringify({
                    v: 1,
                    type: 'call',
                    id: id,
                    name: name,
                    payload: payload || {}
                }));
            });
        },

        on: function(event, callback) {
            if (!this._listeners[event]) {
                this._listeners[event] = [];
            }
            this._listeners[event].push(callback);
            return () => this.off(event, callback);
        },

        off: function(event, callback) {
            if (!this._listeners[event]) {
                return;
            }
            this._listeners[event] = this._listeners[event].filter(fn => fn !== callback);
        }
    };

    window.__kutie_deliver__ = function(envelope) {
        if (!envelope || envelope.v !== 1) {
            return;
        }
        if (envelope.type === 'reply' && envelope.id) {
            const pending = kutie._pending[envelope.id];
            if (!pending) {
                return;
            }
            delete kutie._pending[envelope.id];
            if (envelope.ok) {
                pending.resolve(envelope.data);
            } else {
                pending.reject(new Error(envelope.error || 'IPC call failed'));
            }
            return;
        }
        if (envelope.type === 'event' && envelope.name) {
            const listeners = kutie._listeners[envelope.name] || [];
            listeners.forEach(function(callback) {
                try {
                    callback(envelope.data);
                } catch (error) {
                    console.error('[Kutie] listener error:', error);
                }
            });
        }
    };

    window.chrome.webview.addEventListener('message', function(event) {
        try {
            const raw = typeof event.data === 'string' ? JSON.parse(event.data) : event.data;
            const envelope = typeof raw === 'string' ? JSON.parse(raw) : raw;
            window.__kutie_deliver__(envelope);
        } catch (error) {
            console.error('[Kutie] message parse error:', error);
        }
    });

    function bindDragRegions() {
        document.querySelectorAll('[data-kutie-drag-region]').forEach(function(node) {
            if (node.__kutieDragBound) {
                return;
            }
            node.__kutieDragBound = true;
            const allowMaximize = node.getAttribute('data-kutie-drag-region') !== 'no-maximize';
            const interactiveSelector = 'button, a, input, select, textarea, [data-kutie-no-drag]';
            function isInteractiveTarget(event) {
                return event.target && event.target.closest(interactiveSelector);
            }
            node.addEventListener('mousedown', function(event) {
                if (event.button !== 0 || isInteractiveTarget(event)) {
                    return;
                }
                const startX = event.clientX;
                const startY = event.clientY;
                let dragStarted = false;
                const dragThreshold = 4;

                function cleanup() {
                    document.removeEventListener('mousemove', onMove);
                    document.removeEventListener('mouseup', onUp);
                }

                function onMove(e) {
                    if (dragStarted) {
                        return;
                    }
                    if (Math.abs(e.clientX - startX) >= dragThreshold ||
                        Math.abs(e.clientY - startY) >= dragThreshold) {
                        dragStarted = true;
                        cleanup();
                        kutie.BrowserWindow.getCurrent().startDrag();
                    }
                }

                function onUp() {
                    cleanup();
                }

                document.addEventListener('mousemove', onMove);
                document.addEventListener('mouseup', onUp, { once: true });
            });
            if (allowMaximize) {
                node.addEventListener('dblclick', function(event) {
                    if (isInteractiveTarget(event)) {
                        return;
                    }
                    kutie.BrowserWindow.getCurrent().toggleMaximize();
                });
            }
        });
    }

    kutie.BrowserWindow = {
        getCurrent: function() { return makeWindowApi(windowId); },
        getAll: function() {
            return kutie.call('window.getAll').then(function(result) {
                return (result.ids || []).map(makeWindowApi);
            });
        },
        getFocused: function() {
            return kutie.call('window.getFocused').then(function(result) {
                return result.id ? makeWindowApi(result.id) : null;
            });
        },
        create: function(options) {
            return kutie.call('window.create', options || {}).then(function(result) {
                return makeWindowApi(result.id);
            });
        }
    };

    document.addEventListener('DOMContentLoaded', function() {
        bindDragRegions();
        const observer = new MutationObserver(bindDragRegions);
        observer.observe(document.documentElement, { childList: true, subtree: true });
    });

    window.kutie = kutie;
})(
)js";
    script << window_id;
    script << ");\n";
    return script.str();
}

} // namespace kutie
