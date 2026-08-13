/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglSpine util class header
**
** Wrapper around the official spine-cpp runtime (3rd/spine-runtimes,
** branch 4.2) for playing and rendering Spine skeletal animations.
**
** NOTE: the Spine Runtimes are covered by the Spine Runtimes License
** Agreement - see 3rd/spine-runtimes/LICENSE and
** tutorials/res/spineboy-LICENSE.txt.
*/

#ifndef BSGLSPINE_H
#define BSGLSPINE_H

#include "bsgl.h"

namespace spine {
    class Atlas;
    class Skeleton;
    class SkeletonData;
    class AnimationState;
    class AnimationStateData;
    class AnimationStateListenerObject;
    class Bone;
    class Event;
    class TextureLoader;
}

/*
** Event/complete callbacks (userdata is the pointer passed to the
** corresponding Set...Callback call)
*/
typedef void (*bsglSpineEventCallback)(spine::Event* event, void* userdata);
typedef void (*bsglSpineCompleteCallback)(int trackIndex, int loopCount, void* userdata);

class bsglSpine {
public:
    bsglSpine();
    ~bsglSpine();

    /* Loading (skeleton data may be .json or .skel) */
    bool Load(const char* skeletonFile, const char* atlasFile);

    /* Animation control */
    void SetAnimation(int trackIndex, const char* name, bool loop);
    void AddAnimation(int trackIndex, const char* name, bool loop, float delay=0.0f);
    void SetEmptyAnimation(int trackIndex, float mixDuration);
    void ClearTrack(int trackIndex);
    void ClearTracks();
    const char* GetCurrentAnimation(int trackIndex=0) const;
    bool IsPlaying(int trackIndex=0) const;
    void SetTimeScale(float scale);
    float GetTimeScale() const;

    /* Animation mixing */
    void SetDefaultMix(float duration);
    void SetMix(const char* fromAnimation, const char* toAnimation, float duration);

    /* Transform (position is the screen position of the skeleton origin) */
    void SetPosition(float x, float y);
    void GetPosition(float* x, float* y) const;
    void SetRotation(float degrees);
    float GetRotation() const;
    void SetScale(float scaleX, float scaleY);
    void SetFlip(bool bX, bool bY);
    bool IsFlipX() const;
    bool IsFlipY() const;

    /* Overall tint, applied on top of skeleton/slot colors */
    void SetColor(DWORD color);
    DWORD GetColor() const;

    /* Bone access (IK, attachments, particle emitters etc.) */
    spine::Bone* FindBone(const char* name);
    void GetBoneWorldPosition(const char* boneName, float* x, float* y);

    /* Slots / skins */
    void SetAttachment(const char* slotName, const char* attachmentName);
    void SetSlotColor(const char* slotName, float r, float g, float b, float a);
    void SetSkin(const char* skinName);
    const char* GetSkin() const;

    /* Callbacks */
    void SetEventCallback(bsglSpineEventCallback callback, void* userdata=0);
    void SetCompleteCallback(bsglSpineCompleteCallback callback, void* userdata=0);

    /* Per-frame update (deltaTime in seconds) and render */
    void Update(float deltaTime);
    void Render();

    /* Point test against the current bounding box (screen coordinates) */
    bool HitTest(float x, float y);
    void GetBoundingBox(float* minX, float* minY, float* maxX, float* maxY);

    /* Advanced access to the underlying Spine objects */
    spine::Skeleton* GetSkeleton() const;
    spine::AnimationState* GetAnimationState() const;

private:
    static BSGL* bsgl;

    spine::Atlas*               atlas;
    spine::SkeletonData*        skeletonData;
    spine::Skeleton*            skeleton;
    spine::AnimationStateData*  stateData;
    spine::AnimationState*      state;
    spine::TextureLoader*       texLoader;
    // owned listener, forwards Spine events to the user callbacks
    spine::AnimationStateListenerObject* listener;

    float   posX, posY;
    float   rot;
    float   scaleX, scaleY;
    bool    bXFlip, bYFlip;
    DWORD   color;
    float   timeScale;

    bsglSpineEventCallback      eventCallback;
    void*                       eventUserdata;
    bsglSpineCompleteCallback   completeCallback;
    void*                       completeUserdata;

    /* Spine is Y-up, BSGL is Y-down: this maps skeleton world
    ** coordinates to screen coordinates, applying position, rotation
    ** and the Y flip */
    void WorldToScreen(float wx, float wy, float* sx, float* sy) const;

    friend class BSGLSpineListener;
};

#endif//BSGLSPINE_H
