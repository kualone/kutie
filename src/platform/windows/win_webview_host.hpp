#pragma once

#include <WebView2.h>
#include <functional>
#include <mutex>
#include <vector>

namespace kutie {

class WinWebViewHost {
public:
    using ReadyCallback = std::function<void(HRESULT result, ICoreWebView2Environment* environment)>;

    static WinWebViewHost& Instance();

    void EnsureReady(ReadyCallback callback);
    ICoreWebView2Environment* Environment() const;

private:
    WinWebViewHost() = default;

    mutable std::mutex mutex_;
    ICoreWebView2Environment* environment_ = nullptr;
    bool initializing_ = false;
    std::vector<ReadyCallback> pending_;
};

} // namespace kutie
