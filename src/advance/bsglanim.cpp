/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** bsglAnimation util class implementation
*/

#include "bsglanim.h"

bsglAnimation::bsglAnimation(HTEXTURE tex, int nframes, float FPS, float x, float y, float w, float h)
:   bsglSprite(tex, x, y, w, h) {
    orig_width = bsgl->Texture_GetWidth(tex, true);

    fSinceLastFrame = -1.0f;
    fSpeed = 1.0f / FPS;
    bPlaying = false;
    nFrames = nframes;

    Mode = BSGLANIM_FWD | BSGLANIM_LOOP;
    nDelta = 1;
    SetFrame(0);
}

bsglAnimation::bsglAnimation(const bsglAnimation & anim)
: bsglSprite(anim) {
    // Copy bsglAnimation parameters:
    this->orig_width      = anim.orig_width;
    this->bPlaying        = anim.bPlaying;
    this->fSpeed          = anim.fSpeed;
    this->fSinceLastFrame = anim.fSinceLastFrame;
    this->Mode            = anim.Mode;
    this->nDelta          = anim.nDelta;
    this->nFrames         = anim.nFrames;
    this->nCurFrame       = anim.nCurFrame;
}

void bsglAnimation::Play() {
    bPlaying =true;
    fSinceLastFrame = -1.0f;
    if( Mode & BSGLANIM_REV ) {
        nDelta = -1;
        SetFrame(nFrames-1);
    }else {
        nDelta = 1;
        SetFrame(0);
    }
}

void bsglAnimation::Stop() {
    bPlaying = false;
}

void bsglAnimation:: Resume() {
    bPlaying = true;
}

void bsglAnimation::Update(float fDeltaTime) {
    if( !bPlaying ) {
        return;
    }

    if( fSinceLastFrame == -1.0f ) {
        fSinceLastFrame = 0.0f;
    }else {
        fSinceLastFrame += fDeltaTime;
    }

    while( fSinceLastFrame >= fSpeed ) {
        fSinceLastFrame -= fSpeed;

        if(nCurFrame + nDelta == nFrames) {
            switch(Mode) {
            case BSGLANIM_FWD:
            case BSGLANIM_REV | BSGLANIM_PINGPONG:
                bPlaying = false;
                break;

            case BSGLANIM_FWD | BSGLANIM_PINGPONG:
            case BSGLANIM_FWD | BSGLANIM_PINGPONG | BSGLANIM_LOOP:
            case BSGLANIM_REV | BSGLANIM_PINGPONG | BSGLANIM_LOOP:
                nDelta = -nDelta;
                break;
            }
        }else if( nCurFrame + nDelta < 0 ) {
            switch(Mode) {
            case BSGLANIM_REV:
            case BSGLANIM_FWD | BSGLANIM_PINGPONG:
                bPlaying = false;
                break;

            case BSGLANIM_REV | BSGLANIM_PINGPONG:
            case BSGLANIM_REV | BSGLANIM_PINGPONG | BSGLANIM_LOOP:
            case BSGLANIM_FWD | BSGLANIM_PINGPONG | BSGLANIM_LOOP:
                nDelta = -nDelta;
                break;
            }
        }

        if(bPlaying) {
            SetFrame(nCurFrame+nDelta);
        }
    }
}

bool bsglAnimation::IsPlaying() const {
    return bPlaying;
}

void bsglAnimation::SetTexture(HTEXTURE tex) {
    bsglSprite::SetTexture(tex);
    orig_width = bsgl->Texture_GetWidth(tex, true);
}

void bsglAnimation::SetTextureRect(float x1, float y1, float x2, float y2) {
    bsglSprite::SetTextureRect(x1, y1, x2, y2);
    SetFrame(nCurFrame);
}

void bsglAnimation::SetMode(int mode) {
    Mode = mode;

    if( mode & BSGLANIM_REV ) {
        nDelta = -1;
        SetFrame(nFrames-1);
    }else {
        nDelta = 1;
        SetFrame(0);
    }
}

void bsglAnimation::SetSpeed(float FPS) {
    fSpeed = 1.0f / FPS;
}

void bsglAnimation::SetFrame(int n) {
    float tx1, ty1, tx2, ty2;
    bool bX, bY, bHS;
    int ncols = int(orig_width) / int(width);

    n = n % nFrames;
    if( n < 0 ) {
        n = nFrames + n;
    }
    nCurFrame = n;

    // calculate texture coords for frame n
    ty1 = ty;
    tx1 = tx + n*width;

    if( tx1 > orig_width-width ) {
        n -= int(orig_width-tx) / int(width);
        tx1 = width * (n%ncols);
        ty1 += height * (1 + n/ncols);
    }

    tx2 = tx1 + width;
    ty2 = ty1 + height;

    tx1 /= tex_width;
    ty1 /= tex_height;
    tx2 /= tex_width;
    ty2 /= tex_height;

    quad.v[0].tx=tx1; quad.v[0].ty=ty1;
    quad.v[1].tx=tx2; quad.v[1].ty=ty1;
    quad.v[2].tx=tx2; quad.v[2].ty=ty2;
    quad.v[3].tx=tx1; quad.v[3].ty=ty2;

    bX=bXFlip; bY=bYFlip; bHS=bHSFlip;
    bXFlip=false; bYFlip=false;
    SetFlip(bX,bY,bHS);
}

void bsglAnimation::SetFrames(int n) {
    nFrames = n;
}

int bsglAnimation::GetMode() const {
    return Mode;
}

float bsglAnimation::GetSpeed() const {
    return 1.0f / fSpeed;
}

int bsglAnimation::GetFrame() const {
    return nCurFrame;
}

int bsglAnimation::GetFrames() const {
    return nFrames;
}
