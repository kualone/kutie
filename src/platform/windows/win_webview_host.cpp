#include "win_webview_host.hpp"

#include "win_string_util.hpp"

#include <wrl/event.h>

using Microsoft::WRL::Callback;

namespace kutie {

WinWebViewHost& WinWebViewHost::Instance() {
    static WinWebViewHost host;
    return host;
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

    WCHAR temp_dir[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_dir);
    const std::wstring user_data =
        std::wstring(temp_dir) + L"kutie_webview2_" + std::to_wstring(GetCurrentProcessId());

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
