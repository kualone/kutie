#pragma once

#include "asset_bundle.hpp"
#include "ipc_hub.hpp"
#include "shell.hpp"
#include <functional>
#include <memory>

namespace kutie {

class Runtime {
public:
    struct Config {
        ShellConfig shell;
        /// Optional disk fallback path for development (UTF-8). When empty, uses {exe_dir}/frontend.
        std::string dev_frontend_path;
    };

    using ReadyHandler = std::function<void(Runtime&)>;

    explicit Runtime(const Config& config = {});
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    void OnReady(ReadyHandler handler);
    int Run();

    IpcHub& ipc() { return IpcHub::Shared(); }
    AssetBundle& assets() { return AssetBundle::Shared(); }
    IShell& shell() { return *shell_; }

private:
    void LoadAssets();
    void RegisterBuiltInHandlers();

    Config config_;
    std::unique_ptr<IShell> shell_;
    ReadyHandler ready_handler_;
};

} // namespace kutie
