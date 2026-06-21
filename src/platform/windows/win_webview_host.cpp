#include "win_webview_host.hpp"

#include "win_path_util.hpp"
#include "win_string_util.hpp"

#include <string>
#include <wrl/event.h>

using Microsoft::WRL::Callback;

namespace kutie {

WinWebViewHost& WinWebViewHost::Instance() {
    static WinWebViewHost host;
    return host;
}

void WinWebViewHost::SetUserDataFolder(std::wstring folder) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_data_folder_ = std::move(folder);
}

std::wstring WinWebViewHost::ResolveUserDataFolder() const {
    if (!user_data_folder_.empty()) {
        return user_data_folder_;
    }
    const std::wstring local_app_data = platform::windows::GetLocalAppDataDirectory();
    if (local_app_data.empty()) {
        return L"";
    }
    return local_app_data + L"\\.kutie\\WebView2";
}

ICoreWebView2Environment* WinWebViewHost::Environment() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return environment_;
}

void WinWebViewHost::EnsureReady(ReadyCallback callback) {
    std::vector<ReadyCallback> to_run;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (environment_) {
            to_run.push_back(std::move(callback));
        } else {
            pending_.push_back(std::move(callback));
            if (initializing_) {
                return;
            }
            initializing_ = true;
        }
    }

    if (!to_run.empty()) {
        for (ReadyCallback& ready : to_run) {
            ready(S_OK, environment_);
        }
        return;
    }

    const std::wstring user_data = ResolveUserDataFolder();
    if (user_data.empty() || !platform::windows::EnsureDirectoryExists(user_data)) {
        WinWebViewHost& host = WinWebViewHost::Instance();
        std::vector<ReadyCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(host.mutex_);
            host.initializing_ = false;
            callbacks.swap(host.pending_);
        }
        for (ReadyCallback& ready : callbacks) {
            ready(E_FAIL, nullptr);
        }
        return;
    }

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        user_data.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                WinWebViewHost& host = WinWebViewHost::Instance();
                std::vector<ReadyCallback> callbacks;
                {
                    std::lock_guard<std::mutex> lock(host.mutex_);
                    host.initializing_ = false;
                    if (SUCCEEDED(result) && environment) {
                        host.environment_ = environment;
                        host.environment_->AddRef();
                    }
                    callbacks.swap(host.pending_);
                }
                for (ReadyCallback& callback : callbacks) {
                    callback(result, environment);
                }
                return S_OK;
            })
            .Get());
}

} // namespace kutie
