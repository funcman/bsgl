/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglSpine util class implementation
**
** Bridges the official spine-cpp runtime (3rd/spine-runtimes, branch
** 4.2) and the BSGL renderer:
**
**  - textures are loaded through BSGL::Texture_Load(), so the atlas
**    must reference uncompressed 32-bit BMP files (see
**    tutorials/res/gen_assets.py for the exact format)
**  - region attachments are rendered as bsglQuads, mesh attachments
**    as triangle batches
**  - Spine works in a Y-up coordinate system, BSGL in a Y-down one;
**    bsglSpine::WorldToScreen() does the mapping (plus position and
**    rotation)
**  - BSGL has no multiply/screen blend modes; slots using them fall
**    back to plain alpha blending
**
** The Spine Runtimes are covered by the Spine Runtimes License
** Agreement - see 3rd/spine-runtimes/LICENSE.
*/

#include "bsglspine.h"
#include <spine/spine.h>
#include <math.h>
#include <string.h>

BSGL* bsglSpine::bsgl = 0;

// spine-cpp requires each integration to provide the default extension
// (memory allocation / file reading through the C runtime)
spine::SpineExtension* spine::getDefaultExtension() {
    return new spine::DefaultSpineExtension();
}

//=====================================================================
// Texture loading: spine-cpp asks us to load/unload the texture of
// every atlas page through this interface
//=====================================================================
class BSGLTextureLoader : public spine::TextureLoader {
public:
    BSGLTextureLoader(BSGL* bsgl) : bsgl(bsgl) {}

    void load(spine::AtlasPage& page, const spine::String& path) {
        HTEXTURE tex = bsgl->Texture_Load(path.buffer());
        page.texture = (void*)tex;
        if( tex ) {
            page.width  = bsgl->Texture_GetWidth(tex, true);
            page.height = bsgl->Texture_GetHeight(tex, true);
        }else {
            bsgl->System_Log("bsglSpine: can't load atlas page \"%s\"", path.buffer());
        }
    }

    void unload(void* texture) {
        if( texture ) {
            bsgl->Texture_Free((HTEXTURE)texture);
        }
    }

private:
    BSGL* bsgl;
};

//=====================================================================
// Animation state listener: forwards Spine events to the user callbacks
//=====================================================================
class BSGLSpineListener : public spine::AnimationStateListenerObject {
public:
    BSGLSpineListener(bsglSpine* owner) : owner(owner) {}

    void callback(spine::AnimationState* state, spine::EventType type,
                  spine::TrackEntry* entry, spine::Event* event) {
        (void)state;
        if( spine::EventType_Event == type ) {
            if( owner->eventCallback && event ) {
                owner->eventCallback(event, owner->eventUserdata);
            }
        }else if( spine::EventType_Complete == type ) {
            if( owner->completeCallback && entry ) {
                // loopCount is not exposed by spine-cpp 4.2, pass 0
                owner->completeCallback(entry->getTrackIndex(), 0, owner->completeUserdata);
            }
        }
    }

private:
    bsglSpine* owner;
};

//=====================================================================
// bsglSpine
//=====================================================================
bsglSpine::bsglSpine() {
    if( !bsgl ) {
        bsgl = bsglCreate(BSGL_VERSION);
    }

    atlas        = 0;
    skeletonData = 0;
    skeleton     = 0;
    stateData    = 0;
    state        = 0;
    texLoader    = new BSGLTextureLoader(bsgl);
    listener     = 0;

    posX = posY = 0.0f;
    rot         = 0.0f;
    scaleX = scaleY = 1.0f;
    bXFlip = bYFlip = false;
    color    = RGBA(0xFF, 0xFF, 0xFF, 0xFF);
    timeScale = 1.0f;

    eventCallback      = 0;
    eventUserdata      = 0;
    completeCallback   = 0;
    completeUserdata   = 0;
}

bsglSpine::~bsglSpine() {
    if( state )        { delete state; }
    if( stateData )    { delete stateData; }
    if( skeleton )     { delete skeleton; }
    if( skeletonData ) { delete skeletonData; }
    if( atlas )        { delete atlas; } // also unloads the page textures
    delete texLoader;
    if( listener )     { delete listener; }

    if( bsgl ) {
        bsgl->Release();
        bsgl = 0;
    }
}

bool bsglSpine::Load(const char* skeletonFile, const char* atlasFile) {
    if( skeleton ) {
        bsgl->System_Log("bsglSpine: already loaded");
        return false;
    }

    atlas = new spine::Atlas(atlasFile, texLoader);
    if( atlas->getPages().size() == 0 ) {
        bsgl->System_Log("bsglSpine: can't load atlas \"%s\"", atlasFile);
        delete atlas;
        atlas = 0;
        return false;
    }

    // The skeleton file may be JSON or binary (.skel)
    bool isBinary = false;
    const char* ext = strrchr(skeletonFile, '.');
    if( ext && 0 == strcmp(ext, ".skel") ) {
        isBinary = true;
    }

    spine::String error;
    if( isBinary ) {
        spine::SkeletonBinary binary(atlas);
        skeletonData = binary.readSkeletonDataFile(skeletonFile);
        error = binary.getError();
    }else {
        spine::SkeletonJson json(atlas);
        skeletonData = json.readSkeletonDataFile(skeletonFile);
        error = json.getError();
    }

    if( !skeletonData ) {
        bsgl->System_Log("bsglSpine: can't load skeleton \"%s\": %s",
                         skeletonFile, error.isEmpty() ? "unknown error" : error.buffer());
        delete atlas;
        atlas = 0;
        return false;
    }

    skeleton = new spine::Skeleton(skeletonData);
    skeleton->setToSetupPose();
    // compute world transforms once, so a Render() before the first
    // Update() already shows a valid pose
    skeleton->updateWorldTransform(spine::Physics_Update);

    stateData = new spine::AnimationStateData(skeletonData);
    state     = new spine::AnimationState(stateData);
    listener  = new BSGLSpineListener(this);
    state->setListener(listener);

    return true;
}

//---------------------------------------------------------------------
// Animation control
//---------------------------------------------------------------------
void bsglSpine::SetAnimation(int trackIndex, const char* name, bool loop) {
    if( state ) {
        state->setAnimation(trackIndex, name, loop);
    }
}

void bsglSpine::AddAnimation(int trackIndex, const char* name, bool loop, float delay) {
    if( state ) {
        state->addAnimation(trackIndex, name, loop, delay);
    }
}

void bsglSpine::SetEmptyAnimation(int trackIndex, float mixDuration) {
    if( state ) {
        state->setEmptyAnimation(trackIndex, mixDuration);
    }
}

void bsglSpine::ClearTrack(int trackIndex) {
    if( state ) {
        state->clearTrack(trackIndex);
    }
}

void bsglSpine::ClearTracks() {
    if( state ) {
        state->clearTracks();
    }
}

const char* bsglSpine::GetCurrentAnimation(int trackIndex) const {
    if( state ) {
        spine::TrackEntry* entry = state->getCurrent(trackIndex);
        if( entry && entry->getAnimation() ) {
            return entry->getAnimation()->getName().buffer();
        }
    }
    return "";
}

bool bsglSpine::IsPlaying(int trackIndex) const {
    if( state ) {
        spine::TrackEntry* entry = state->getCurrent(trackIndex);
        if( entry ) {
            return entry->getLoop() || entry->getTrackTime() < entry->getAnimationEnd();
        }
    }
    return false;
}

void bsglSpine::SetTimeScale(float scale) {
    timeScale = scale;
}

float bsglSpine::GetTimeScale() const {
    return timeScale;
}

void bsglSpine::SetDefaultMix(float duration) {
    if( stateData ) {
        stateData->setDefaultMix(duration);
    }
}

void bsglSpine::SetMix(const char* fromAnimation, const char* toAnimation, float duration) {
    if( stateData ) {
        stateData->setMix(fromAnimation, toAnimation, duration);
    }
}

//---------------------------------------------------------------------
// Transform
//---------------------------------------------------------------------
void bsglSpine::SetPosition(float x, float y) {
    posX = x;
    posY = y;
}

void bsglSpine::GetPosition(float* x, float* y) const {
    *x = posX;
    *y = posY;
}

void bsglSpine::SetRotation(float degrees) {
    rot = degrees;
}

float bsglSpine::GetRotation() const {
    return rot;
}

void bsglSpine::SetScale(float sx, float sy) {
    scaleX = sx;
    scaleY = sy;
    if( skeleton ) {
        skeleton->setScaleX(scaleX * (bXFlip ? -1.0f : 1.0f));
        skeleton->setScaleY(scaleY * (bYFlip ? -1.0f : 1.0f));
    }
}

void bsglSpine::SetFlip(bool bX, bool bY) {
    bXFlip = bX;
    bYFlip = bY;
    if( skeleton ) {
        skeleton->setScaleX(scaleX * (bXFlip ? -1.0f : 1.0f));
        skeleton->setScaleY(scaleY * (bYFlip ? -1.0f : 1.0f));
    }
}

bool bsglSpine::IsFlipX() const {
    return bXFlip;
}

bool bsglSpine::IsFlipY() const {
    return bYFlip;
}

void bsglSpine::SetColor(DWORD col) {
    color = col;
}

DWORD bsglSpine::GetColor() const {
    return color;
}

//---------------------------------------------------------------------
// Bones / slots / skins
//---------------------------------------------------------------------
spine::Bone* bsglSpine::FindBone(const char* name) {
    return skeleton ? skeleton->findBone(name) : 0;
}

void bsglSpine::GetBoneWorldPosition(const char* boneName, float* x, float* y) {
    spine::Bone* bone = FindBone(boneName);
    if( bone ) {
        WorldToScreen(bone->getWorldX(), bone->getWorldY(), x, y);
    }else {
        *x = posX;
        *y = posY;
    }
}

void bsglSpine::SetAttachment(const char* slotName, const char* attachmentName) {
    if( skeleton ) {
        skeleton->setAttachment(slotName, attachmentName);
    }
}

void bsglSpine::SetSlotColor(const char* slotName, float r, float g, float b, float a) {
    if( skeleton ) {
        spine::Slot* slot = skeleton->findSlot(slotName);
        if( slot ) {
            slot->getColor().set(r, g, b, a);
        }
    }
}

void bsglSpine::SetSkin(const char* skinName) {
    if( skeleton ) {
        skeleton->setSkin(skinName);
        skeleton->setSlotsToSetupPose();
    }
}

const char* bsglSpine::GetSkin() const {
    if( skeleton && skeleton->getSkin() ) {
        return skeleton->getSkin()->getName().buffer();
    }
    return "";
}

//---------------------------------------------------------------------
// Callbacks
//---------------------------------------------------------------------
void bsglSpine::SetEventCallback(bsglSpineEventCallback callback, void* userdata) {
    eventCallback = callback;
    eventUserdata = userdata;
}

void bsglSpine::SetCompleteCallback(bsglSpineCompleteCallback callback, void* userdata) {
    completeCallback = callback;
    completeUserdata = userdata;
}

//---------------------------------------------------------------------
// Update / Render
//---------------------------------------------------------------------
void bsglSpine::Update(float deltaTime) {
    if( !skeleton ) {
        return;
    }

    state->update(deltaTime * timeScale);
    state->apply(*skeleton);
    skeleton->updateWorldTransform(spine::Physics_Update);
}

void bsglSpine::WorldToScreen(float wx, float wy, float* sx, float* sy) const {
    float rad = rot * float(M_PI) / 180.0f;
    float c   = cosf(rad);
    float s   = sinf(rad);
    float x   = wx;
    float y   = -wy;            // Spine is Y-up, BSGL is Y-down
    *sx = posX + x*c - y*s;
    *sy = posY + x*s + y*c;
}

// Spine blend mode -> BSGL blend flags (multiply/screen unsupported)
static int SpineBlendToBSGL(spine::BlendMode mode) {
    switch( mode ) {
    case spine::BlendMode_Additive:
        return BLEND_COLORMUL | BLEND_ALPHAADD | BLEND_NOZWRITE;
    case spine::BlendMode_Multiply:
    case spine::BlendMode_Screen:
    case spine::BlendMode_Normal:
    default:
        return BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
    }
}

void bsglSpine::Render() {
    if( !skeleton ) {
        return;
    }

    float tr = float(GETR(color)) / 255.0f;
    float tg = float(GETG(color)) / 255.0f;
    float tb = float(GETB(color)) / 255.0f;
    float ta = float(GETA(color)) / 255.0f;

    spine::Vector<spine::Slot*>& drawOrder = skeleton->getDrawOrder();
    for( size_t i=0; i<drawOrder.size(); ++i ) {
        spine::Slot* slot = drawOrder[i];
        spine::Attachment* attachment = slot->getAttachment();
        if( !attachment ) {
            continue;
        }

        // combined vertex color: skeleton * slot * attachment * user tint
        spine::Color& skelCol = skeleton->getColor();
        spine::Color& slotCol = slot->getColor();

        if( attachment->getRTTI().instanceOf(spine::RegionAttachment::rtti) ) {
            spine::RegionAttachment* region = (spine::RegionAttachment*)attachment;
            spine::AtlasRegion* atlasRegion = (spine::AtlasRegion*)region->getRegion();
            if( !atlasRegion || !atlasRegion->page || !atlasRegion->page->texture ) {
                continue;
            }

            float world[8];
            region->computeWorldVertices(*slot, world, 0, 2);

            spine::Color& attCol = region->getColor();
            unsigned char cr = (unsigned char)(skelCol.r * slotCol.r * attCol.r * tr * 255.0f);
            unsigned char cg = (unsigned char)(skelCol.g * slotCol.g * attCol.g * tg * 255.0f);
            unsigned char cb = (unsigned char)(skelCol.b * slotCol.b * attCol.b * tb * 255.0f);
            unsigned char ca = (unsigned char)(skelCol.a * slotCol.a * attCol.a * ta * 255.0f);

            // Spine vertex order is BL(0), UL(1), UR(2), BR(3);
            // bsglQuad wants v[0]=TL, v[1]=TR, v[2]=BR, v[3]=BL
            static const int order[4] = { 1, 2, 3, 0 };
            spine::Vector<float>& uvs = region->getUVs();

            bsglQuad quad;
            quad.tex   = (HTEXTURE)atlasRegion->page->texture;
            quad.blend = SpineBlendToBSGL(slot->getData().getBlendMode());
            for( int k=0; k<4; ++k ) {
                int v = order[k];
                WorldToScreen(world[v*2], world[v*2+1], &quad.v[k].x, &quad.v[k].y);
                quad.v[k].z     = 0.5f;
                quad.v[k].tx    = uvs[v*2];
                quad.v[k].ty    = uvs[v*2+1];
                quad.v[k].red   = cr;
                quad.v[k].green = cg;
                quad.v[k].blue  = cb;
                quad.v[k].alpha = ca;
            }
            bsgl->Gfx_RenderQuad(&quad);
        }
        else if( attachment->getRTTI().instanceOf(spine::MeshAttachment::rtti) ) {
            spine::MeshAttachment* mesh = (spine::MeshAttachment*)attachment;
            spine::AtlasRegion* atlasRegion = (spine::AtlasRegion*)mesh->getRegion();
            if( !atlasRegion || !atlasRegion->page || !atlasRegion->page->texture ) {
                continue;
            }

            size_t worldLen = (size_t)mesh->getWorldVerticesLength();
            spine::Vector<float> world;
            world.setSize(worldLen, 0.0f);
            mesh->computeWorldVertices(*slot, 0, worldLen, world.buffer(), 0, 2);

            spine::Color& attCol = mesh->getColor();
            unsigned char cr = (unsigned char)(skelCol.r * slotCol.r * attCol.r * tr * 255.0f);
            unsigned char cg = (unsigned char)(skelCol.g * slotCol.g * attCol.g * tg * 255.0f);
            unsigned char cb = (unsigned char)(skelCol.b * slotCol.b * attCol.b * tb * 255.0f);
            unsigned char ca = (unsigned char)(skelCol.a * slotCol.a * attCol.a * ta * 255.0f);

            spine::Vector<float>&          uvs      = mesh->getUVs(); // final page texture coordinates
            spine::Vector<unsigned short>& triangles = mesh->getTriangles();
            HTEXTURE tex   = (HTEXTURE)atlasRegion->page->texture;
            int      blend = SpineBlendToBSGL(slot->getData().getBlendMode());

            // submit the mesh as triangle batches, chunking if it does
            // not fit into the vertex buffer at once
            size_t nTri  = triangles.size() / 3;
            size_t done  = 0;
            while( done < nTri ) {
                int maxPrim = 0;
                bsglVertex* vb = bsgl->Gfx_StartBatch(BSGLPRIM_TRIPLES, tex, blend, &maxPrim);
                if( !vb || !maxPrim ) {
                    break;
                }
                int chunk = int(nTri - done);
                if( chunk > maxPrim ) {
                    chunk = maxPrim;
                }
                bsglVertex* v = vb;
                for( int t=0; t<chunk; ++t ) {
                    for( int k=0; k<3; ++k ) {
                        int idx = triangles[(done+t)*3+k];
                        WorldToScreen(world[idx*2], world[idx*2+1], &v->x, &v->y);
                        v->z     = 0.5f;
                        v->tx    = uvs[idx*2];
                        v->ty    = uvs[idx*2+1];
                        v->red   = cr;
                        v->green = cg;
                        v->blue  = cb;
                        v->alpha = ca;
                        ++v;
                    }
                }
                done += chunk;
                bsgl->Gfx_FinishBatch(chunk);
            }
        }
        // clipping attachments are not supported by BSGL (no stencil);
        // spineboy does not use them
    }
}

//---------------------------------------------------------------------
// Bounds / picking
//---------------------------------------------------------------------
void bsglSpine::GetBoundingBox(float* minX, float* minY, float* maxX, float* maxY) {
    *minX = *minY = *maxX = *maxY = 0.0f;
    if( !skeleton ) {
        return;
    }

    static spine::Vector<float> buffer;
    float x, y, w, h;
    skeleton->getBounds(x, y, w, h, buffer);

    // transform the 4 world corners and take the screen-space extents
    float sx, sy;
    *minX = *maxX = posX;
    *minY = *maxY = posY;
    const float cx[4] = { x, x+w, x+w, x };
    const float cy[4] = { y, y, y+h, y+h };
    for( int i=0; i<4; ++i ) {
        WorldToScreen(cx[i], cy[i], &sx, &sy);
        if( sx < *minX ) *minX = sx;
        if( sx > *maxX ) *maxX = sx;
        if( sy < *minY ) *minY = sy;
        if( sy > *maxY ) *maxY = sy;
    }
}

bool bsglSpine::HitTest(float x, float y) {
    float minX, minY, maxX, maxY;
    GetBoundingBox(&minX, &minY, &maxX, &maxY);
    return x >= minX && x <= maxX && y >= minY && y <= maxY;
}

//---------------------------------------------------------------------
// Advanced access
//---------------------------------------------------------------------
spine::Skeleton* bsglSpine::GetSkeleton() const {
    return skeleton;
}

spine::AnimationState* bsglSpine::GetAnimationState() const {
    return state;
}
