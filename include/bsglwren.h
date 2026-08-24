/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglWren util class header
**
** Hosts a Wren VM (3rd/wren) and binds the BSGL API to Wren so that
** games can be written entirely in Wren. The embedded Wren library
** (module "bsgl") exposes System/Gfx/Input/Timer/Random/Config,
** Texture/Sprite/Animation/Font/Spine/DBones/Widget classes and the
** Blend/Color/Key/Anim constants.
**
** Typical host program:
**
**     void bsgl_main() {
**         bsglWren wren;
**         wren.Run("main.wren");
**     }
**
** The script sets up the system state, calls System.initiate(),
** creates a global `var main = Main.new()` whose update(dt)/render()
** methods drive the frame loop, then calls System.start().
*/

#ifndef BSGLWREN_H
#define BSGLWREN_H

class bsglWrenImpl;

class bsglWren {
public:
    bsglWren();
    ~bsglWren();

    /* Compiles and runs a Wren script (module "main"). The script is
    ** expected to drive the whole lifecycle itself via the bsgl
    ** module; Run() returns when the script top-level finishes. */
    bool Run(char const* scriptFile);

    /* Last Wren compile/runtime error, or an empty string */
    char const* GetLastError() const;

private:
    bsglWren(const bsglWren&);
    bsglWren& operator=(const bsglWren&);

    bsglWrenImpl* impl;
};

#endif//BSGLWREN_H
