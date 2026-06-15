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

`tools/embed_assets.py` scans `sample/frontend/` and generates:

- `build/generated/assets.rc` — Windows resources (`KUTIE_ASSET`)
- `build/generated/asset_loader.cpp` — `AssetBundle::LoadEmbedded()`
- `build/generated/asset_ids.h` — numeric resource IDs (starting at 2000)

CMake runs the script when frontend files change.

## Runtime Loading Order

1. `AssetBundle::LoadEmbedded(GetModuleHandle(nullptr))`
2. If empty, search for `sample/frontend` or `./frontend` relative to the executable
3. `AssetBundle::LoadFromDisk(path)`

No code changes between dev and prod.

## SPA Fallback

When a request path is not found and does not look like a static asset (`.js`, `.css`, `.png`, …), Kutie serves `/index.html`.

## Manual Registration

```cpp
app.assets().Put("/api/mock.json", R"({"ok":true})", "application/json");
```

## MIME Types

Inferred from file extension when not specified. Override with the third argument to `Put()`.

## Cross-platform Note

Phase 2+ may add a portable `embedded_assets.cpp` generator. Windows currently uses `.rc` embedding.
