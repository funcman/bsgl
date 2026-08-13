/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglDBones util class header
**
** Wrapper around the DragonBonesCPP runtime (3rd/DragonBonesCPP,
** version 5.6.300) for playing and rendering DragonBones skeletal
** animations.
**
** NOTE: DragonBonesCPP is released under the MIT license - see
** 3rd/DragonBonesCPP/LICENSE.
*/

#ifndef BSGLDBONES_H
#define BSGLDBONES_H

#include "bsgl.h"

namespace dragonBones {
    class DragonBones;
    class BaseFactory;
    class Armature;
    class Animation;
    class IArmatureProxy;
}

class bsglDBones {
public:
    bsglDBones();
    ~bsglDBones();

    /* Loading: skeleton JSON + texture-atlas JSON + the atlas texture
    ** image itself. dataName caches the parsed data under this name;
    ** armatureName 0 = use the first armature in the skeleton data. */
    bool Load(const char* skeletonFile, const char* texAtlasFile, const char* texImageFile,
              const char* dataName="default", const char* armatureName=0);

    /* Animation control */
    void Play(const char* name, int playTimes=-1);   // 0 loops forever
    void FadeIn(const char* name, float fadeTime, int playTimes=-1);
    bool HasAnimation(const char* name) const;
    int  GetAnimationCount() const;
    const char* GetAnimationName(int index) const;
    const char* GetCurrentAnimation() const;
    void SetTimeScale(float scale);
    float GetTimeScale() const;

    /* Transform (position is the screen position of the armature origin) */
    void SetPosition(float x, float y);
    void GetPosition(float* x, float* y) const;
    void SetScale(float scaleX, float scaleY);
    void SetFlip(bool bX, bool bY);
    bool IsFlipX() const;
    bool IsFlipY() const;

    /* Overall tint, applied on top of slot colors */
    void SetColor(DWORD color);
    DWORD GetColor() const;

    /* Per-frame update (deltaTime in seconds) and render */
    void Update(float deltaTime);
    void Render();

    /* Advanced access */
    dragonBones::Armature* GetArmature() const;

private:
    static BSGL* bsgl;

    dragonBones::DragonBones*       dragonBonesInstance;
    dragonBones::BaseFactory*       factory;
    dragonBones::Armature*          armature;
    dragonBones::IArmatureProxy*    proxy;
    HTEXTURE                        texture;

    float   posX, posY;
    float   scaleX, scaleY;
    bool    bXFlip, bYFlip;
    DWORD   color;
    float   timeScale;

    /* DragonBones is Y-down just like BSGL: this maps armature world
    ** coordinates to screen coordinates, applying position, scale and
    ** the flips (no Y flip, unlike bsglSpine) */
    void WorldToScreen(float wx, float wy, float* sx, float* sy) const;
};

#endif//BSGLDBONES_H
