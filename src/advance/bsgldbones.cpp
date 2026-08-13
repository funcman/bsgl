/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglDBones util class implementation
**
** Bridges the DragonBonesCPP runtime (3rd/DragonBonesCPP, version
** 5.6.300) and the BSGL renderer:
**
**  - the atlas texture is loaded through BSGL::Texture_Load(), so it
**    must be in a format Texture_Load supports (the tutorial uses an
**    uncompressed 32-bit BMP)
**  - image displays are rendered as bsglQuads, mesh displays as
**    triangle batches
**  - DragonBones works in a Y-down coordinate system, just like BSGL
**    (DragonBones::yDown defaults to true), so unlike bsglSpine no Y
**    flip is needed; the slot globalTransformMatrix is applied
**    verbatim and bsglDBones::WorldToScreen() only adds position,
**    scale and the flips
**  - BSGL has no multiply/screen blend modes; slots using them fall
**    back to plain alpha blending
**
** DragonBonesCPP is released under the MIT license - see
** 3rd/DragonBonesCPP/LICENSE.
*/

#include "bsgldbones.h"
#include <dragonBones/DragonBonesHeaders.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <string>
#include <vector>

BSGL* bsglDBones::bsgl = nullptr;

//=====================================================================
// Event dispatcher: the runtime requires one, but bsglDBones does not
// forward DragonBones events anywhere
//=====================================================================
class BSGLDBEventDispatcher : public dragonBones::IEventDispatcher {
public:
    virtual bool hasDBEventListener(const std::string& type) const override {
        (void)type;
        return true;
    }

    virtual void addDBEventListener(const std::string& type,
                                    const std::function<void(dragonBones::EventObject*)>& listener) override {
        (void)type; (void)listener;
    }

    virtual void removeDBEventListener(const std::string& type,
                                       const std::function<void(dragonBones::EventObject*)>& listener) override {
        (void)type; (void)listener;
    }

    virtual void dispatchDBEvent(const std::string& type, dragonBones::EventObject* value) override {
        (void)type; (void)value;
    }
};

//=====================================================================
// Texture data: carries the BSGL texture handle of the atlas
//=====================================================================
class BSGLDBTextureData : public dragonBones::TextureData {
    BIND_CLASS_TYPE_B(BSGLDBTextureData);

public:
    HTEXTURE texture;

    BSGLDBTextureData() {
        _onClear();
    }

    virtual ~BSGLDBTextureData() {
        _onClear();
    }

    virtual void _onClear() override {
        texture = 0;
        TextureData::_onClear();
    }
};

class BSGLDBTextureAtlasData : public dragonBones::TextureAtlasData {
    BIND_CLASS_TYPE_B(BSGLDBTextureAtlasData);

public:
    HTEXTURE texture;

    BSGLDBTextureAtlasData() {
        _onClear();
    }

    virtual ~BSGLDBTextureAtlasData() {
        _onClear();
    }

    virtual dragonBones::TextureData* createTexture() const override {
        return dragonBones::BaseObject::borrowObject<BSGLDBTextureData>();
    }

    // hands the atlas texture over to every region of the atlas;
    // rotated regions get their width/height swapped so that the
    // region always holds the display (unrotated) size
    void SetRenderTexture(HTEXTURE value) {
        if (texture == value) {
            return;
        }

        texture = value;

        if (texture) {
            for( auto& pair : textures ) {
                auto textureData = static_cast<BSGLDBTextureData*>(pair.second);
                if (textureData->texture == 0) {
                    dragonBones::Rectangle region;
                    region.x      = textureData->region.x;
                    region.y      = textureData->region.y;
                    region.width  = textureData->rotated ? textureData->region.height : textureData->region.width;
                    region.height = textureData->rotated ? textureData->region.width  : textureData->region.height;

                    textureData->texture = texture;
                    textureData->region  = region;
                }
            }
        }
    }

protected:
    virtual void _onClear() override {
        texture = 0;
        TextureAtlasData::_onClear();
    }
};

//=====================================================================
// Armature proxy: the runtime talks to the engine through this
// interface; BSGL renders immediate-mode, so it is just a stub
// holding the armature pointer
//=====================================================================
class BSGLDBArmatureProxy : public dragonBones::IArmatureProxy {
public:
    BSGLDBArmatureProxy() : armature(nullptr) {}

    virtual void dbInit(dragonBones::Armature* armature_) override {
        armature = armature_;
    }

    virtual void dbClear() override {
        armature = nullptr;
    }

    virtual void dbUpdate() override {
    }

    virtual void dispose(bool disposeProxy) override {
        (void)disposeProxy;
        if (armature) {
            armature->dispose();
            armature = nullptr;
        }
    }

    virtual dragonBones::Armature* getArmature() const override {
        return armature;
    }

    virtual dragonBones::Animation* getAnimation() const override {
        return armature ? armature->getAnimation() : nullptr;
    }

    // IEventDispatcher (events are not forwarded anywhere)
    virtual bool hasDBEventListener(const std::string& type) const override {
        (void)type;
        return true;
    }

    virtual void addDBEventListener(const std::string& type,
                                    const std::function<void(dragonBones::EventObject*)>& listener) override {
        (void)type; (void)listener;
    }

    virtual void removeDBEventListener(const std::string& type,
                                       const std::function<void(dragonBones::EventObject*)>& listener) override {
        (void)type; (void)listener;
    }

    virtual void dispatchDBEvent(const std::string& type, dragonBones::EventObject* value) override {
        (void)type; (void)value;
    }

private:
    dragonBones::Armature* armature;
};

//=====================================================================
// Slot: BSGL renders immediate-mode, so most display-management hooks
// are empty; the renderer reads the runtime state back through the
// Get... accessors
//=====================================================================
class BSGLDBSlot : public dragonBones::Slot {
    BIND_CLASS_TYPE_A(BSGLDBSlot);

public:
    // dummy display handles passed to Slot::init(): the display objects
    // carry no state of their own, but the runtime requires distinct,
    // stable pointers (it compares them to tell image/mesh displays
    // apart and to detect display changes)
    void* RawDisplayTag()  { return &rawDisplayTag; }
    void* MeshDisplayTag() { return &meshDisplayTag; }

    const dragonBones::TextureData* GetTextureData() const { return _textureData; }
    float GetPivotX() const { return _pivotX; }
    float GetPivotY() const { return _pivotY; }
    const dragonBones::ColorTransform& GetColorTransform() const { return _colorTransform; }
    dragonBones::BlendMode GetBlendMode() const { return _blendMode; }
    const dragonBones::DeformVertices* GetDeformVertices() const { return _deformVertices; }

    virtual void _updateVisible() override {
    }

    virtual void _updateBlendMode() override {
        if (_childArmature) {
            for( const auto slot : _childArmature->getSlots() ) {
                slot->_blendMode = _blendMode;
                slot->_updateBlendMode();
            }
        }
    }

    virtual void _updateColor() override {
    }

protected:
    virtual void _initDisplay(void* value, bool isRetain) override {
        (void)value; (void)isRetain;
    }

    virtual void _disposeDisplay(void* value, bool isRelease) override {
        (void)value; (void)isRelease;
    }

    virtual void _onUpdateDisplay() override {
    }

    virtual void _addDisplay() override {
    }

    virtual void _replaceDisplay(void* value, bool isArmatureDisplay) override {
        (void)value; (void)isArmatureDisplay;
    }

    virtual void _removeDisplay() override {
    }

    virtual void _updateZOrder() override {
    }

    virtual void _updateFrame() override {
        // nothing to cache for immediate-mode rendering, but this hook
        // must run when the texture data changes so that the dependent
        // state is refreshed by Slot::update()
        if (_displayIndex >= 0 && _display != nullptr && _textureData != nullptr) {
            _visibleDirty   = true;
            _blendModeDirty = true;
            _colorDirty     = true;
        }
    }

    virtual void _updateMesh() override {
        // mesh deformation is recomputed every frame in Render()
    }

    virtual void _updateTransform() override {
        // Render() reads globalTransformMatrix directly
    }

    virtual void _identityTransform() override {
    }

    virtual void _onClear() override {
        dragonBones::Slot::_onClear();
    }

private:
    int rawDisplayTag;
    int meshDisplayTag;
};

//=====================================================================
// Factory: parses the data buffers and builds armatures/slots;
// each bsglDBones instance owns one factory and one DragonBones
// engine instance
//=====================================================================
class BSGLDBFactory : public dragonBones::BaseFactory {
public:
    BSGLDBFactory() {
        eventDispatcher = new BSGLDBEventDispatcher();
        _dragonBones    = new dragonBones::DragonBones(eventDispatcher);
    }

    virtual ~BSGLDBFactory() {
        // the DragonBones instance is deleted by bsglDBones, after the
        // factory; its destructor does not touch the event dispatcher
        delete eventDispatcher;
    }

    dragonBones::DragonBones* GetDragonBones() const {
        return _dragonBones;
    }

protected:
    virtual dragonBones::TextureAtlasData* _buildTextureAtlasData(
            dragonBones::TextureAtlasData* textureAtlasData, void* textureAtlas) const override {
        auto atlasData = static_cast<BSGLDBTextureAtlasData*>(textureAtlasData);
        if (atlasData != nullptr) {
            if (textureAtlas != nullptr) {
                atlasData->SetRenderTexture((HTEXTURE)textureAtlas);
            } else {
                DRAGONBONES_ASSERT(false, "No atlas texture");
            }
        } else {
            atlasData = dragonBones::BaseObject::borrowObject<BSGLDBTextureAtlasData>();
        }
        return atlasData;
    }

    virtual dragonBones::Armature* _buildArmature(const dragonBones::BuildArmaturePackage& dataPackage) const override {
        const auto armature = dragonBones::BaseObject::borrowObject<dragonBones::Armature>();
        const auto proxy    = new BSGLDBArmatureProxy();

        armature->init(dataPackage.armature, proxy, proxy, _dragonBones);

        return armature;
    }

    virtual dragonBones::Slot* _buildSlot(const dragonBones::BuildArmaturePackage& dataPackage,
                                          const dragonBones::SlotData* slotData,
                                          dragonBones::Armature* armature) const override {
        (void)dataPackage;
        auto slot = dragonBones::BaseObject::borrowObject<BSGLDBSlot>();
        slot->init(slotData, armature, slot->RawDisplayTag(), slot->MeshDisplayTag());

        return slot;
    }

private:
    BSGLDBEventDispatcher* eventDispatcher;
};

//=====================================================================
// bsglDBones
//=====================================================================
bsglDBones::bsglDBones() {
    if (!bsgl) {
        bsgl = bsglCreate(BSGL_VERSION);
    }

    factory            = new BSGLDBFactory();
    dragonBonesInstance = static_cast<BSGLDBFactory*>(factory)->GetDragonBones();
    armature           = nullptr;
    proxy              = nullptr;
    texture            = 0;

    posX = posY = 0.0f;
    scaleX = scaleY = 1.0f;
    bXFlip = bYFlip = false;
    color     = RGBA(0xFF, 0xFF, 0xFF, 0xFF);
    timeScale = 1.0f;
}

bsglDBones::~bsglDBones() {
    // never delete armatures/slots directly: they live in the
    // runtime's object pool, dispose() returns them there
    if (armature) {
        armature->dispose();
    }
    // the armature proxy is a plain object, not pooled
    delete static_cast<BSGLDBArmatureProxy*>(proxy);
    if (factory) {
        factory->clear();
        delete factory;
    }
    delete dragonBonesInstance;

    if (texture) {
        bsgl->Texture_Free(texture);
    }

    if (bsgl) {
        bsgl->Release();
        bsgl = nullptr;
    }
}

// Reads a whole file into a freshly malloc'ed, 0-terminated buffer
static char* ReadFileContents(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (buffer) {
        if (size > 0 && fread(buffer, 1, size, f) != (size_t)size) {
            free(buffer);
            buffer = nullptr;
        } else {
            buffer[size] = '\0';
        }
    }

    fclose(f);
    return buffer;
}

bool bsglDBones::Load(const char* skeletonFile, const char* texAtlasFile, const char* texImageFile,
                      const char* dataName, const char* armatureName) {
    if (armature) {
        bsgl->System_Log("bsglDBones: already loaded");
        return false;
    }

    texture = bsgl->Texture_Load(texImageFile);
    if (!texture) {
        bsgl->System_Log("bsglDBones: can't load texture \"%s\"", texImageFile);
        return false;
    }

    char* skeletonData = ReadFileContents(skeletonFile);
    if (!skeletonData) {
        bsgl->System_Log("bsglDBones: can't load skeleton \"%s\"", skeletonFile);
        bsgl->Texture_Free(texture);
        texture = 0;
        return false;
    }

    dragonBones::DragonBonesData* dbData = factory->parseDragonBonesData(skeletonData, dataName, 1.0f);
    free(skeletonData);
    if (!dbData) {
        bsgl->System_Log("bsglDBones: can't parse skeleton \"%s\"", skeletonFile);
        bsgl->Texture_Free(texture);
        texture = 0;
        return false;
    }

    char* atlasData = ReadFileContents(texAtlasFile);
    if (!atlasData) {
        bsgl->System_Log("bsglDBones: can't load texture atlas \"%s\"", texAtlasFile);
        factory->clear();
        bsgl->Texture_Free(texture);
        texture = 0;
        return false;
    }

    // the opaque atlas pointer is passed to _buildTextureAtlasData(),
    // which hands the texture over to the atlas regions
    dragonBones::TextureAtlasData* texAtlasData =
        factory->parseTextureAtlasData(atlasData, (void*)texture, dataName, 1.0f);
    free(atlasData);
    if (!texAtlasData) {
        bsgl->System_Log("bsglDBones: can't parse texture atlas \"%s\"", texAtlasFile);
        factory->clear();
        bsgl->Texture_Free(texture);
        texture = 0;
        return false;
    }

    // armatureName 0 = the first armature in the skeleton data
    std::string armName;
    if (armatureName) {
        armName = armatureName;
    } else if (!dbData->getArmatureNames().empty()) {
        armName = dbData->getArmatureNames().front();
    }

    if (armName.empty()) {
        bsgl->System_Log("bsglDBones: no armature in \"%s\"", skeletonFile);
        factory->clear();
        bsgl->Texture_Free(texture);
        texture = 0;
        return false;
    }

    armature = factory->buildArmature(armName, dataName);
    if (!armature) {
        bsgl->System_Log("bsglDBones: can't build armature \"%s\"", armName.c_str());
        factory->clear();
        bsgl->Texture_Free(texture);
        texture = 0;
        return false;
    }

    proxy = static_cast<dragonBones::IArmatureProxy*>(armature->getDisplay());

    dragonBonesInstance->getClock()->add(armature);
    armature->getAnimation()->timeScale = timeScale;

    // advance once, so a Render() before the first Update() already
    // shows a valid pose
    dragonBonesInstance->advanceTime(0.0f);

    return true;
}

//---------------------------------------------------------------------
// Animation control
//---------------------------------------------------------------------
void bsglDBones::Play(const char* name, int playTimes) {
    if (armature) {
        armature->getAnimation()->play(name, playTimes);
    }
}

void bsglDBones::FadeIn(const char* name, float fadeTime, int playTimes) {
    if (armature) {
        armature->getAnimation()->fadeIn(name, fadeTime, playTimes);
    }
}

bool bsglDBones::HasAnimation(const char* name) const {
    return armature ? armature->getAnimation()->hasAnimation(name) : false;
}

int bsglDBones::GetAnimationCount() const {
    if (armature) {
        return (int)armature->getAnimation()->getAnimationNames().size();
    }
    return 0;
}

const char* bsglDBones::GetAnimationName(int index) const {
    if (armature) {
        const std::vector<std::string>& names = armature->getAnimation()->getAnimationNames();
        if (index >= 0 && index < (int)names.size()) {
            return names[index].c_str();
        }
    }
    return "";
}

const char* bsglDBones::GetCurrentAnimation() const {
    if (armature) {
        return armature->getAnimation()->getLastAnimationName().c_str();
    }
    return "";
}

void bsglDBones::SetTimeScale(float scale) {
    timeScale = scale;
    if (armature) {
        armature->getAnimation()->timeScale = scale;
    }
}

float bsglDBones::GetTimeScale() const {
    return timeScale;
}

//---------------------------------------------------------------------
// Transform
//---------------------------------------------------------------------
void bsglDBones::SetPosition(float x, float y) {
    posX = x;
    posY = y;
}

void bsglDBones::GetPosition(float* x, float* y) const {
    *x = posX;
    *y = posY;
}

void bsglDBones::SetScale(float sx, float sy) {
    scaleX = sx;
    scaleY = sy;
}

void bsglDBones::SetFlip(bool bX, bool bY) {
    bXFlip = bX;
    bYFlip = bY;
}

bool bsglDBones::IsFlipX() const {
    return bXFlip;
}

bool bsglDBones::IsFlipY() const {
    return bYFlip;
}

void bsglDBones::SetColor(DWORD col) {
    color = col;
}

DWORD bsglDBones::GetColor() const {
    return color;
}

//---------------------------------------------------------------------
// Update / Render
//---------------------------------------------------------------------
void bsglDBones::Update(float deltaTime) {
    if (!armature) {
        return;
    }

    dragonBonesInstance->advanceTime(deltaTime);
}

void bsglDBones::WorldToScreen(float wx, float wy, float* sx, float* sy) const {
    // DragonBones is Y-down just like BSGL, no flip needed here
    *sx = posX + wx * scaleX * (bXFlip ? -1.0f : 1.0f);
    *sy = posY + wy * scaleY * (bYFlip ? -1.0f : 1.0f);
}

// DragonBones blend mode -> BSGL blend flags (multiply/screen etc.
// unsupported, they fall back to plain alpha blending)
static int DBBlendToBSGL(dragonBones::BlendMode mode) {
    switch( mode ) {
    case dragonBones::BlendMode::Add:
        return BLEND_COLORMUL | BLEND_ALPHAADD | BLEND_NOZWRITE;
    default:
        return BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
    }
}

void bsglDBones::Render() {
    if (!armature) {
        return;
    }

    float tr = float(GETR(color)) / 255.0f;
    float tg = float(GETG(color)) / 255.0f;
    float tb = float(GETB(color)) / 255.0f;
    float ta = float(GETA(color)) / 255.0f;

    const float armatureScale = armature->getArmatureData()->scale;
    const std::vector<dragonBones::Slot*>& slots = armature->getSlots(); // z-ordered

    for( size_t i=0; i<slots.size(); ++i ) {
        BSGLDBSlot* slot = static_cast<BSGLDBSlot*>(slots[i]);

        // slots with no render display (displayIndex < 0) have a null
        // display; child-armature displays have no texture data
        const BSGLDBTextureData* texData = static_cast<const BSGLDBTextureData*>(slot->GetTextureData());
        if (!slot->getVisible() || !slot->getDisplay() || !texData || !texData->texture) {
            continue;
        }

        // combined vertex color: slot color * user tint
        const dragonBones::ColorTransform& ct = slot->GetColorTransform();
        unsigned char cr = (unsigned char)(ct.redMultiplier   * tr * 255.0f);
        unsigned char cg = (unsigned char)(ct.greenMultiplier * tg * 255.0f);
        unsigned char cb = (unsigned char)(ct.blueMultiplier  * tb * 255.0f);
        unsigned char ca = (unsigned char)(ct.alphaMultiplier * ta * 255.0f);

        HTEXTURE tex   = texData->texture;
        int      blend = DBBlendToBSGL(slot->GetBlendMode());

        float texW = (float)bsgl->Texture_GetWidth(tex, true);
        float texH = (float)bsgl->Texture_GetHeight(tex, true);

        const dragonBones::Rectangle& region = texData->region;
        const dragonBones::Matrix&    m      = slot->globalTransformMatrix;

        bool isMesh = slot->getDisplay() == slot->getMeshDisplay()
                   && slot->GetDeformVertices() != nullptr
                   && slot->GetDeformVertices()->verticesData != nullptr;

        if (!isMesh) {
            // image display: a quad, pivot-subtracted and transformed
            // by the slot's global matrix
            float scale = texData->parent->scale * armatureScale;
            float w = region.width  * scale;
            float h = region.height * scale;

            float ox = m.tx - (m.a*slot->GetPivotX() + m.c*slot->GetPivotY());
            float oy = m.ty - (m.b*slot->GetPivotX() + m.d*slot->GetPivotY());

            float u0 = region.x / texW;
            float v0 = region.y / texH;
            float u1 = (region.x + region.width)  / texW;
            float v1 = (region.y + region.height) / texH;

            // bsglQuad wants v[0]=TL, v[1]=TR, v[2]=BR, v[3]=BL
            const float cx[4] = { 0, w, w, 0 };
            const float cy[4] = { 0, 0, h, h };
            const float cu[4] = { u0, u1, u1, u0 };
            const float cv[4] = { v0, v0, v1, v1 };

            bsglQuad quad;
            quad.tex   = tex;
            quad.blend = blend;
            for( int k=0; k<4; ++k ) {
                WorldToScreen(ox + m.a*cx[k] + m.c*cy[k],
                              oy + m.b*cx[k] + m.d*cy[k],
                              &quad.v[k].x, &quad.v[k].y);
                quad.v[k].z     = 0.5f;
                quad.v[k].tx    = cu[k];
                quad.v[k].ty    = cv[k];
                quad.v[k].red   = cr;
                quad.v[k].green = cg;
                quad.v[k].blue  = cb;
                quad.v[k].alpha = ca;
            }
            bsgl->Gfx_RenderQuad(&quad);
        } else {
            // mesh display: deform the vertices and submit the mesh as
            // triangle batches, chunking if it does not fit into the
            // vertex buffer at once
            const dragonBones::DeformVertices* deform       = slot->GetDeformVertices();
            const dragonBones::VerticesData*   verticesData = deform->verticesData;
            const int16_t* intArray   = verticesData->data->intArray;
            const float*   floatArray = verticesData->data->floatArray;

            const unsigned vertexCount   = (unsigned)intArray[verticesData->offset + (unsigned)dragonBones::BinaryOffset::MeshVertexCount];
            const unsigned triangleCount = (unsigned)intArray[verticesData->offset + (unsigned)dragonBones::BinaryOffset::MeshTriangleCount];
            int vertexOffset = intArray[verticesData->offset + (unsigned)dragonBones::BinaryOffset::MeshFloatOffset];
            if (vertexOffset < 0) {
                vertexOffset += 65536;
            }
            const unsigned uvOffset = (unsigned)vertexOffset + vertexCount * 2;

            const std::vector<float>& deformVerts = deform->vertices;
            const bool hasFFD  = !deformVerts.empty();
            const bool skinned = verticesData->weight != nullptr;

            std::vector<float> vx(vertexCount), vy(vertexCount);
            std::vector<float> vu(vertexCount), vv(vertexCount);

            if (skinned) {
                // skinned mesh: blend the bone matrices (the slot
                // matrix is identity in this case)
                const dragonBones::WeightData* weightData = verticesData->weight;
                const std::vector<dragonBones::Bone*>& bones = deform->bones;
                int weightFloatOffset = intArray[weightData->offset + (unsigned)dragonBones::BinaryOffset::WeigthFloatOffset];
                if (weightFloatOffset < 0) {
                    weightFloatOffset += 65536;
                }

                size_t iB = weightData->offset + (unsigned)dragonBones::BinaryOffset::WeigthBoneIndices + bones.size();
                size_t iV = (size_t)weightFloatOffset;
                size_t iF = 0;
                for( unsigned v=0; v<vertexCount; ++v ) {
                    const unsigned boneCount = (unsigned)intArray[iB++];
                    float xG = 0.0f, yG = 0.0f;
                    for( unsigned j=0; j<boneCount; ++j ) {
                        const unsigned boneIndex = (unsigned)intArray[iB++];
                        const dragonBones::Bone* bone = bones[boneIndex];
                        if (bone != nullptr) {
                            const dragonBones::Matrix& bm = bone->globalTransformMatrix;
                            const float weight = floatArray[iV++];
                            float xL = floatArray[iV++] * armatureScale;
                            float yL = floatArray[iV++] * armatureScale;

                            if (hasFFD) {
                                xL += deformVerts[iF++];
                                yL += deformVerts[iF++];
                            }

                            xG += (bm.a * xL + bm.c * yL + bm.tx) * weight;
                            yG += (bm.b * xL + bm.d * yL + bm.ty) * weight;
                        }
                    }
                    WorldToScreen(xG, yG, &vx[v], &vy[v]);
                }
            } else {
                // plain mesh (optionally with free-form deformation):
                // the slot matrix still applies
                for( unsigned v=0; v<vertexCount; ++v ) {
                    float xL = floatArray[vertexOffset + v*2]     * armatureScale;
                    float yL = floatArray[vertexOffset + v*2 + 1] * armatureScale;
                    if (hasFFD) {
                        xL += deformVerts[v*2];
                        yL += deformVerts[v*2 + 1];
                    }
                    WorldToScreen(m.a*xL + m.c*yL + m.tx,
                                  m.b*xL + m.d*yL + m.ty,
                                  &vx[v], &vy[v]);
                }
            }

            for( unsigned v=0; v<vertexCount; ++v ) {
                const float u = floatArray[uvOffset + v*2];
                const float t = floatArray[uvOffset + v*2 + 1];
                if (texData->rotated) {
                    vu[v] = (region.x + (1.0f - t) * region.width)  / texW;
                    vv[v] = (region.y + u * region.height) / texH;
                } else {
                    vu[v] = (region.x + u * region.width)  / texW;
                    vv[v] = (region.y + t * region.height) / texH;
                }
            }

            const unsigned indexOffset = verticesData->offset + (unsigned)dragonBones::BinaryOffset::MeshVertexIndices;
            size_t done = 0;
            while (done < triangleCount) {
                int maxPrim = 0;
                bsglVertex* vb = bsgl->Gfx_StartBatch(BSGLPRIM_TRIPLES, tex, blend, &maxPrim);
                if (!vb || !maxPrim) {
                    break;
                }
                int chunk = int(triangleCount - done);
                if (chunk > maxPrim) {
                    chunk = maxPrim;
                }
                bsglVertex* v = vb;
                for( int t=0; t<chunk; ++t ) {
                    for( int k=0; k<3; ++k ) {
                        unsigned idx = (unsigned)intArray[indexOffset + (done+t)*3+k];
                        v->x     = vx[idx];
                        v->y     = vy[idx];
                        v->z     = 0.5f;
                        v->tx    = vu[idx];
                        v->ty    = vv[idx];
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
    }
}

//---------------------------------------------------------------------
// Advanced access
//---------------------------------------------------------------------
dragonBones::Armature* bsglDBones::GetArmature() const {
    return armature;
}
