#pragma once

#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace kutie {

class IpcHub {
public:
    using Handler = std::function<nlohmann::json(const nlohmann::json& payload)>;
    using ScriptRunner = std::function<void(const std::string& script)>;

    static IpcHub& Shared();

    IpcHub(const IpcHub&) = delete;
    IpcHub& operator=(const IpcHub&) = delete;

    void RegisterHandler(const std::string& name, Handler handler);
    void UnregisterHandler(const std::string& name);

    template <typename ArgsT, typename ResultT>
    void RegisterHandler(const std::string& name, std::function<ResultT(const ArgsT&)> handler) {
        RegisterHandler(name, [handler = std::move(handler)](const nlohmann::json& payload) -> nlohmann::json {
            ArgsT args = payload.get<ArgsT>();
            ResultT result = handler(args);
            return nlohmann::json(result);
        });
    }

    template <typename ResultT>
    void RegisterHandler(const std::string& name, std::function<ResultT()> handler) {
        RegisterHandler(name, [handler = std::move(handler)](const nlohmann::json&) -> nlohmann::json {
            return nlohmann::json(handler());
        });
    }

    std::string DispatchCall(int id, const std::string& name, const std::string& payload_json);
    void Broadcast(const std::string& event, const nlohmann::json& data);

    void SetScriptRunner(ScriptRunner runner);
    static std::string ClientBootstrapScript();

private:
    IpcHub() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Handler> handlers_;
    ScriptRunner script_runner_;
};

} // namespace kutie
