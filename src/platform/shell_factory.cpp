#include "kutie/platform/shell_factory.hpp"

#include "windows/win_shell.hpp"

namespace kutie {

std::unique_ptr<IShell> CreateShell(const ShellConfig& config, AssetBundle& assets, IpcHub& ipc) {
#if defined(KUTIE_PLATFORM_WINDOWS)
    return std::make_unique<WinShell>(config, assets, ipc);
#else
    (void)config;
    (void)assets;
    (void)ipc;
    return nullptr;
#endif
}

} // namespace kutie
