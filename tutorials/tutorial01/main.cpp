/*
** BSGL Tutorial 01 - Minimal BSGL application
** (adapted from hge_tut01 - Minimal HGE application)
**
** A BSGL program does not define main() itself: the library provides
** the platform entry point and calls bsgl_main(). All you have to do
** is implement bsgl_main().
**
** This tutorial shows the smallest possible application:
** initiate the system, run the loop, and quit when ESC is pressed.
** Rendering is covered from tutorial 02 on.
*/

#include "bsgl.h"

// Global pointer to the BSGL interface.
// Instead you may call bsglCreate() every time you need access
// to the interface. Just be sure to have a corresponding
// bsgl->Release() for each call to bsglCreate().
static BSGL* bsgl = nullptr;

// This function will be called by BSGL once per logic frame.
// Put your game loop code here. In this example we
// just check whether ESC key has been pressed.
bool LogicFunc() {
    // Input is sampled once per logic frame with Control_GetState(),
    // afterwards the state of each key can be queried.
    bsgl->Control_GetState();

    // By returning "true" we tell BSGL
    // to stop running the application.
    if (bsgl->Control_IsDown(INP_ESC)) {
        return true;
    }

    // Continue execution
    return false;
}

void bsgl_main() {
    // Here we use the global pointer to the BSGL interface.
    bsgl = bsglCreate(BSGL_VERSION);

    // Set our logic function
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);

    // Set the window title
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 01 - Minimal BSGL application");

    // Run in windowed mode
    // Default window size is 800x600
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);

    // Tries to initiate BSGL with the states set.
    // If something goes wrong, "false" is returned
    // and a more specific description of what has
    // happened can be read with System_GetErrorMessage().
    if (bsgl->System_Initiate()) {
        // Starts running LogicFunc().
        // Note that the execution "stops" here
        // until "true" is returned from LogicFunc().
        bsgl->System_Start();

        // Now ESC has been pressed or the user
        // has closed the window by other means.
        // Free all allocated resources.
        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    // Release the BSGL interface.
    // If there are no more references,
    // the BSGL object will be deleted.
    bsgl->Release();
}
