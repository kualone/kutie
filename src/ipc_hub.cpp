#include "kutie/ipc_hub.hpp"

#include <sstream>

namespace kutie {

IpcHub& IpcHub::Shared() {
    static IpcHub instance;
    return instance;
}

void IpcHub::RegisterHandler(const std::string& name, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[name] = std::move(handler);
}

void IpcHub::UnregisterHandler(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(name);
}

std::string IpcHub::DispatchCall(int id, const std::string& name, const std::string& payload_json) {
    Handler handler;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = handlers_.find(name);
        if (it == handlers_.end()) {
            nlohmann::json envelope;
            envelope["v"] = 1;
            envelope["type"] = "reply";
            envelope["id"] = id;
            envelope["ok"] = false;
            envelope["error"] = "Unknown handler: " + name;
            return envelope.dump();
        }
        handler = it->second;
    }

    try {
        nlohmann::json payload = payload_json.empty() ? nlohmann::json::object() : nlohmann::json::parse(payload_json);
        nlohmann::json data = handler(payload);

        nlohmann::json envelope;
        envelope["v"] = 1;
        envelope["type"] = "reply";
        envelope["id"] = id;
        envelope["ok"] = true;
        envelope["data"] = std::move(data);
        return envelope.dump();
    } catch (const std::exception& ex) {
        nlohmann::json envelope;
        envelope["v"] = 1;
        envelope["type"] = "reply";
        envelope["id"] = id;
        envelope["ok"] = false;
        envelope["error"] = ex.what();
        return envelope.dump();
    }
}

void IpcHub::Broadcast(const std::string& event, const nlohmann::json& data) {
    ScriptRunner runner;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        runner = script_runner_;
    }
    if (!runner) {
        return;
    }

    nlohmann::json envelope;
    envelope["v"] = 1;
    envelope["type"] = "event";
    envelope["name"] = event;
    envelope["data"] = data;

    runner("window.__kutie_deliver__(" + envelope.dump() + ");");
}

void IpcHub::SetScriptRunner(ScriptRunner runner) {
    std::lock_guard<std::mutex> lock(mutex_);
    script_runner_ = std::move(runner);
}

std::string IpcHub::ClientBootstrapScript() {
    return R"js(
(function() {
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
        },

        window: {
            startDrag: function() {
                return kutie.call('shell.start_drag');
            }
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
            node.addEventListener('mousedown', function(event) {
                if (event.button !== 0) {
                    return;
                }
                kutie.window.startDrag();
            });
            if (allowMaximize) {
                node.addEventListener('dblclick', function() {
                    kutie.call('shell.toggle_maximize');
                });
            }
        });
    }

    const observer = new MutationObserver(bindDragRegions);
    document.addEventListener('DOMContentLoaded', function() {
        bindDragRegions();
        observer.observe(document.documentElement, { childList: true, subtree: true });
    });

    window.kutie = kutie;
})();
)js";
}

} // namespace kutie
