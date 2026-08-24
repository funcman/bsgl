/*
** BSGL Tutorial 10 - Writing games in Wren with bsglWren
**
** The whole tutorial lives in main.wren: the C++ side only hosts the
** Wren VM. The script sets up the system state, defines a `Main`
** class with init()/update(dt)/render() and assigns the global
** `var main`; bsglWren::Run() then drives the whole lifecycle
** (initiate, init, frame loop, teardown).
**
** The demo shows a sprite, a sprite animation, FPS text via bsglFont
** and (if the resource files are found) a Spine and a DragonBones
** skeleton. Press ESC to quit.
*/

#include "bsgl.h"
#include "bsglwren.h"

void bsgl_main() {
    BSGL* bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial10.log");

    bsglWren wren;
    if (!wren.Run("main.wren")) {
        bsgl->System_Log("tutorial10: %s", wren.GetLastError());
    }

    bsgl->Release();
}
