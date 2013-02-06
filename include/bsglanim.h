/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** bsglAnimation util class header
*/

#ifndef BSGLANIM_H
#define BSGLANIM_H

#include "bsglsprite.h"

#define BSGLANIM_FWD        0
#define BSGLANIM_REV        1
#define BSGLANIM_PINGPONG   2
#define BSGLANIM_NOPINGPONG 0
#define BSGLANIM_LOOP       4
#define BSGLANIM_NOLOOP     0

class bsglAnimation : public bsglSprite {
public:
    bsglAnimation(HTEXTURE tex, int nframes, float FPS, float x, float y, float w, float h);
    bsglAnimation(const bsglAnimation& anim);

    void Play();
    void Stop();
    void Resume();
    void Update(float fDeltaTime);
    bool IsPlaying() const;

    void SetTexture(HTEXTURE tex);
    void SetTextureRect(float x1, float y1, float x2, float y2);
    void SetMode(int mode);
    void SetSpeed(float FPS);
    void SetFrame(int n);
    void SetFrames(int n);

    int GetMode() const;
    float GetSpeed() const;
    int GetFrame() const;
    int GetFrames() const;

private:
    bsglAnimation();

    int orig_width;

    bool bPlaying;

    float fSpeed;
    float fSinceLastFrame;

    int Mode;
    int nDelta;
    int nFrames;
    int nCurFrame;
};

#endif//BSGLANIM_H
