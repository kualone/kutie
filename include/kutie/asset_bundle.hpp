#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace kutie {

class AssetBundle {
public:
    struct Asset {
        std::vector<std::uint8_t> bytes;
        std::string content_type;
    };

    using EmbeddedLoader = std::function<void(void* module_handle, AssetBundle& bundle)>;

    static AssetBundle& Shared();
    static void RegisterEmbeddedLoader(EmbeddedLoader loader);

    AssetBundle(const AssetBundle&) = delete;
    AssetBundle& operator=(const AssetBundle&) = delete;

    void Put(const std::string& path, const std::vector<std::uint8_t>& bytes, const std::string& content_type = {});
    void Put(const std::string& path, const std::string& text, const std::string& content_type = {});
    void Put(const std::string& path, std::vector<std::uint8_t>&& bytes, const std::string& content_type = {});

    const Asset* Get(const std::string& path) const;
    bool Resolve(const std::string& path, Asset& out) const;

    void LoadEmbedded(void* module_handle);
    void LoadFromDisk(const std::string& root_dir);
    void Clear();

    std::vector<std::string> Paths() const;
    bool Empty() const;

    static std::string NormalizePath(std::string path);
    static std::string GuessContentType(const std::string& path);
    static bool LooksLikeStaticAsset(const std::string& path);

private:
    AssetBundle() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Asset> assets_;
};

} // namespace kutie
