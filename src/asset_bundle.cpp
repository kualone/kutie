#include "kutie/asset_bundle.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace kutie {

namespace {

struct ExtensionInfo {
    const char* ext;
    const char* mime;
};

const ExtensionInfo kExtensionTable[] = {
    {".css", "text/css; charset=utf-8"},
    {".gif", "image/gif"},
    {".htm", "text/html; charset=utf-8"},
    {".html", "text/html; charset=utf-8"},
    {".ico", "image/x-icon"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "application/javascript; charset=utf-8"},
    {".json", "application/json; charset=utf-8"},
    {".map", "application/json; charset=utf-8"},
    {".otf", nullptr},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain; charset=utf-8"},
    {".wasm", "application/wasm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xml", nullptr},
};

const ExtensionInfo* FindExtensionInfo(const std::string& ext) {
    for (const ExtensionInfo& info : kExtensionTable) {
        if (_stricmp(ext.c_str(), info.ext) == 0) {
            return &info;
        }
    }
    return nullptr;
}

std::vector<AssetBundle::EmbeddedLoader>& EmbeddedLoaders() {
    static std::vector<AssetBundle::EmbeddedLoader> loaders;
    return loaders;
}

} // namespace

AssetBundle& AssetBundle::Shared() {
    static AssetBundle instance;
    return instance;
}

void AssetBundle::RegisterEmbeddedLoader(EmbeddedLoader loader) {
    EmbeddedLoaders().push_back(std::move(loader));
}

void AssetBundle::LoadEmbedded(void* module_handle) {
    for (const auto& loader : EmbeddedLoaders()) {
        loader(module_handle, *this);
    }
}

std::string AssetBundle::NormalizePath(std::string path) {
    if (path.empty()) {
        path = "/";
    }
    if (path.front() != '/') {
        path.insert(path.begin(), '/');
    }
    std::replace(path.begin(), path.end(), '\\', '/');
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

std::string AssetBundle::GuessContentType(const std::string& path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    const std::string ext = path.substr(dot);
    if (const ExtensionInfo* info = FindExtensionInfo(ext)) {
        if (info->mime) {
            return info->mime;
        }
    }
    return "application/octet-stream";
}

bool AssetBundle::LooksLikeStaticAsset(const std::string& path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos) {
        return false;
    }
    return FindExtensionInfo(path.substr(dot)) != nullptr;
}

void AssetBundle::Put(const std::string& path, const std::vector<std::uint8_t>& bytes, const std::string& content_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    Asset asset;
    asset.bytes = bytes;
    asset.content_type = content_type.empty() ? GuessContentType(path) : content_type;
    assets_[NormalizePath(path)] = std::move(asset);
}

void AssetBundle::Put(const std::string& path, const std::string& text, const std::string& content_type) {
    Put(path, std::vector<std::uint8_t>(text.begin(), text.end()), content_type);
}

void AssetBundle::Put(const std::string& path, std::vector<std::uint8_t>&& bytes, const std::string& content_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    Asset asset;
    asset.bytes = std::move(bytes);
    asset.content_type = content_type.empty() ? GuessContentType(path) : content_type;
    assets_[NormalizePath(path)] = std::move(asset);
}

const AssetBundle::Asset* AssetBundle::Get(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = assets_.find(NormalizePath(path));
    return it == assets_.end() ? nullptr : &it->second;
}

bool AssetBundle::Resolve(const std::string& path, Asset& out) const {
    if (const Asset* asset = Get(path)) {
        out = *asset;
        return true;
    }
    return false;
}

void AssetBundle::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    assets_.clear();
}

std::vector<std::string> AssetBundle::Paths() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> paths;
    paths.reserve(assets_.size());
    for (const auto& entry : assets_) {
        paths.push_back(entry.first);
    }
    return paths;
}

bool AssetBundle::Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return assets_.empty();
}

void AssetBundle::LoadFromDisk(const std::string& root_dir) {
    namespace fs = std::filesystem;
    const fs::path root(root_dir);
    if (!fs::exists(root)) {
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::ifstream stream(entry.path(), std::ios::binary);
        if (!stream) {
            continue;
        }

        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};

        const std::string relative = entry.path().lexically_relative(root).generic_string();
        Put("/" + relative, bytes);
    }
}

} // namespace kutie
