# Asset Bundling

Kutie serves frontend files from memory in production and from disk in development.

## Virtual Origin

Assets are loaded from:

```
https://assets.kutie/index.html
https://assets.kutie/styles/main.css
https://assets.kutie/scripts/app.js
```

Configure via `ShellConfig::entry_url` (default above).

## Build-time Embedding

Each application embeds its own frontend. The Kutie library does **not** ship app assets.

`tools/embed_assets.py` scans your `frontend/` directory and generates:

- `assets.rc` — Windows resources (`KUTIE_ASSET`)
- `kutie_embedded_assets.cpp` — registers an `AssetBundle` embedded loader at static init
- `asset_ids.h` — numeric resource IDs (starting at 2000)

Use the CMake helper (in-tree or after `find_package(Kutie)`):

```cmake
kutie_embed_frontend(myapp "${CMAKE_SOURCE_DIR}/frontend")
```

Generated code calls `AssetBundle::RegisterEmbeddedLoader()` so resources are loaded from the **executable** module handle.

## Runtime Loading Order

1. `AssetBundle::LoadEmbedded(GetModuleHandle(nullptr))` — runs registered loaders
2. If empty, load from disk:
   - `Runtime::Config::dev_frontend_path` when set
   - otherwise `{exe_dir}/frontend`
3. `AssetBundle::LoadFromDisk(path)`

No C++ code changes between dev and prod.

## SPA Fallback

When a request path is not found and does not look like a static asset (`.js`, `.css`, `.png`, …), Kutie serves `/index.html`.

## Manual Registration

```cpp
app.assets().Put("/api/mock.json", R"({"ok":true})", "application/json");
```

## MIME Types

Inferred from file extension when not specified. Override with the third argument to `Put()`.

## Cross-platform Note

Phase 2+ may add a portable embedded asset generator. Windows currently uses `.rc` embedding.

See also [packaging.md](packaging.md) for `find_package(Kutie)` integration.
