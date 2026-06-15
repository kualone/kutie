#pragma once

#include "kutie/shell.hpp"
#include <memory>

namespace kutie {

std::unique_ptr<IShell> CreateShell(const ShellConfig& config, AssetBundle& assets, IpcHub& ipc);

} // namespace kutie
