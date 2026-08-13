# BSGL Tutorials

A tutorial series for BSGL, adapted from the
[HGE 1.9 tutorials](https://github.com/kvakvs/hge/tree/hge1.9/tutorials)
to the features BSGL actually provides. (HGE tutorials covering sound,
particles, render targets and distortion meshes have no BSGL
counterpart and were replaced by tutorials on BSGL-specific topics.)

| Tutorial | Topic |
| --- | --- |
| tutorial01 | Minimal BSGL application: `bsgl_main()`, system states, the main loop |
| tutorial02 | Input and rendering: `bsglQuad`, textures, blending, keyboard & mouse |
| tutorial03 | Helper classes: `bsglSprite` and `bsglAnimation` |
| tutorial04 | Rendering text with `bsglFont` (FreeType) |
| tutorial05 | Config file (`Config_*`), random numbers, timer and logging |
| tutorial06 | Creating a simple menu with `bsglWidget` |
| tutorial07 | Thousand of Hares: sprite batching stress test, blend modes |
| tutorial08 | Spine skeletal animation with `bsglSpine` (spine-cpp runtime) |

## How it works

A BSGL application does **not** define `main()` itself. The library
provides the platform entry point and calls `bsgl_main()`. Each
tutorial is a single `main.cpp` that defines `bsgl_main()` and links
against the `bsgl` static library.

The minimal skeleton looks like this:

```cpp
#include "bsgl.h"

static BSGL* bsgl = 0;

bool LogicFunc()  { /* return true to quit */ }
bool RenderFunc() { /* drawing code; return false */ }

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    if (bsgl->System_Initiate()) {
        bsgl->System_Start();
        bsgl->System_Shutdown();
    }
    bsgl->Release();
}
```

## Building

The tutorials are built by default together with the library
(`BSGL_BUILD_TUTORIALS=ON`). Use the one-click scripts from the
repository root:

```
scripts\build.ps1            # Windows (PowerShell)
scripts/build.sh             # Linux / macOS
```

The executables land in `build/bin/<Config>` (e.g.
`build/bin/Release/tutorial01.exe`), next to `SDL3.dll` and the
tutorial resources (`sprite.bmp`, `anim.bmp` are copied there
automatically). Run them from that directory.

To skip building the tutorials:

```
cmake -S . -B build -DBSGL_BUILD_TUTORIALS=OFF
```

## Notes

- **Controls.** Every tutorial quits with ESC. Tutorial 02/03 move an
  orb with the arrow keys; 03 toggles animation playback with SPACE;
  06 is mouse-driven.
- **Input model.** Call `Control_GetState()` once per logic frame,
  then query keys with `Control_IsDown()` (pressed this frame),
  `Control_IsPassing()` (held) and `Control_IsUp()` (released this
  frame).
- **Textures.** `Texture_Load()` picks the decoder by file name
  suffix: `.png` (libspng) and `.jpg`/`.jpeg` (libjpeg-turbo) are
  decoded directly; anything else is read as an uncompressed 32-bit
  BMP. PNG keeps its alpha channel, JPEG does not. The tutorial
  artwork is generated procedurally — see `res/gen_assets.py`
  (run `python res/gen_assets.py` to regenerate).
- **tutorial07 hare sprite.** The sprite texture is `res/mushroom.bmp`
  (an uncompressed 32-bit `BI_RGB` BMP, 64x64 with alpha). Swap in any
  other 64x64 BMP of the same format to change the hare; tutorial07
  renders the top left 64x64 pixels of the loaded texture.
- **Fonts.** Tutorials 04 and 06 need a TrueType font: they try
  `font.ttf` in the working directory first, then fall back to the
  Windows system Arial. Copy any `.ttf` next to the executable to use
  your own font. Without a font, tutorial 06 still works — the buttons
  just show no labels.
- **Config.** Tutorial 05 creates/updates `config.xml` in the working
  directory. `System_Initiate()` also reads the `[Screen]`, `[FPS]`
  and `[Window]` sections from that file, so you can change window
  size, title and frame rates without recompiling.
- **Spine (tutorial08).** Plays the official `spineboy` example via
  the spine-cpp runtime, pulled in as the `3rd/spine-runtimes`
  submodule (branch 4.2 — the runtime version must match the version
  the data was exported with). The atlas references `spineboy.bmp`,
  an uncompressed 32-bit BMP converted from the official
  `spineboy.png`, because `Texture_Load()` only reads that format.
  The spineboy assets are for evaluation/learning only and may not be
  used commercially; distributing software containing the Spine
  Runtimes requires a Spine Editor license. See
  `res/spineboy-LICENSE.txt` and `3rd/spine-runtimes/LICENSE`.
