BSGL - Buggy-Mushroom's Simple Game Library
====

A OpenGL port of HGE, running on SDL3.

## Building

Third-party dependencies (SDL3, freetype, tinyxml2) are git submodules
under `3rd/` and are built automatically by CMake. The one-click
scripts initialize the submodules, configure and build:

```
scripts\build.ps1            # Windows (PowerShell)
scripts/build.sh             # Linux / macOS
```

Or by hand:

```
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --config Release
```

The static library lands in `build/lib/<Config>`, the SDL3 runtime in
`build/bin/<Config>`.

## Using the library

Link against `bsgl` and define `bsgl_main()` — the library provides
the platform entry point:

```cpp
#include "bsgl.h"

void bsgl_main() {
    BSGL* bsgl = bsglCreate(BSGL_VERSION);
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    if (bsgl->System_Initiate()) {
        bsgl->System_Start();
        bsgl->System_Shutdown();
    }
    bsgl->Release();
}
```

## Tutorials

A tutorial series adapted from the HGE 1.9 tutorials lives in
`tutorials/`. It is built by default; disable it with
`-DBSGL_BUILD_TUTORIALS=OFF`. See `tutorials/README.md` for details.
