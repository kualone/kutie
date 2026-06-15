#include "kutie/asset_bundle.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace kutie {

namespace {

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring result(static_cast<std::size_t>(length - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
    return result;
}

} // namespace

AssetBundle& AssetBundle::Shared() {
    static AssetBundle instance;
    return instance;
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
    std::string ext = path.substr(dot);
    for (auto& ch : ext) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".webp") return "image/webp";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf") return "font/ttf";
    if (ext == ".txt") return "text/plain; charset=utf-8";
    if (ext == ".wasm") return "application/wasm";
    if (ext == ".map") return "application/json; charset=utf-8";
    return "application/octet-stream";
}

bool AssetBundle::LooksLikeStaticAsset(const std::string& path) {
    static const char* extensions[] = {
        ".js", ".css", ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico",
        ".woff", ".woff2", ".ttf", ".otf", ".wasm", ".json", ".xml",
        ".txt", ".webp", ".map", nullptr
    };

    const auto dot = path.rfind('.');
    if (dot == std::string::npos) {
        return false;
    }

    const std::string ext = path.substr(dot);
    for (const char** current = extensions; *current; ++current) {
        if (_stricmp(ext.c_str(), *current) == 0) {
            return true;
        }
    }
    return false;
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
