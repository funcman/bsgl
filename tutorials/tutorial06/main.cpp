/*
** BSGL Tutorial 06 - Creating a simple menu with bsglWidget
** (adapted from hge_tut06 - Creating menus)
**
** bsglWidget is a minimal GUI building block: a colored rectangle
** with a position, a list of child widgets, mouse hit-testing and
** four overridable callbacks (OnRender/OnOver/OnDown/OnUp).
** Children use coordinates relative to their parent.
**
** This tutorial builds a panel with three labelled buttons:
**   - "Color" cycles the background color,
**   - "Log"   writes a line to the log,
**   - "Quit"  quits the application.
**
** The button labels are text textures rendered with bsglFont,
** exactly as shown in tutorial 04, using the bundled DejaVu Sans
** font (tutorials/res/font.ttf). Without the font the buttons
** simply show no text.
** Pressing ESC also quits.
*/

#include "bsgl.h"
#include "bsglsprite.h"
#include "bsglfont.h"
#include "bsglwidget.h"
#include <stdio.h>
#include <string.h>

static BSGL* bsgl = 0;

static bool        quit_requested = false;
static DWORD       bg_colors[] = {
    RGBA(0x20, 0x20, 0x40, 0xFF),
    RGBA(0x40, 0x20, 0x20, 0xFF),
    RGBA(0x20, 0x40, 0x20, 0xFF),
};
static int         bg_index = 0;

// A simple push button: highlights under the cursor,
// darkens while pressed, fires OnUp() when released inside
// and draws a text label on top of its background.
class Button : public bsglWidget {
public:
    Button(int x, int y, int w, int h, int id, bsglSprite* label)
    :   bsglWidget(x, y, w, h)
    ,   id_(id)
    ,   label_(label) {
        SetBackgroundColor(RGBA(0x70, 0x70, 0x90, 0xFF));
    }

    ~Button() {
        delete label_;
    }

    void OnRender(float x, float y) {
        // draw the background quad first, the label on top
        bsglWidget::OnRender(x, y);
        if( label_ ) {
            label_->Render(x_+x, y_+y);
        }
    }

    void OnOver(float x, float y) {
        if( TestAt(x, y) ) {
            SetBackgroundColor(RGBA(0xB0, 0xB0, 0xD0, 0xFF));
        }else {
            SetBackgroundColor(RGBA(0x70, 0x70, 0x90, 0xFF));
        }
    }

    void OnDown() {
        SetBackgroundColor(RGBA(0x50, 0x50, 0x70, 0xFF));
    }

    void OnUp(bool inside) {
        if( !inside ) {
            return;
        }
        switch( id_ ) {
            case 1:
                bg_index = (bg_index + 1) % 3;
                break;
            case 2:
                bsgl->System_Log("Button 2 clicked.");
                printf("Button 2 clicked.\n");
                break;
            case 3:
                quit_requested = true;
                break;
            default:
                break;
        }
    }

private:
    int         id_;
    bsglSprite* label_;
};

static bsglWidget* panel = 0;

// The bundled DejaVu Sans font (tutorials/res/font.ttf), copied next
// to the executable at build time - see tutorials/CMakeLists.txt.
static const char* const FONT_FILE = "font.ttf";

static bool FontFileExists() {
    FILE* f = fopen(FONT_FILE, "rb");
    if( f ) {
        fclose(f);
        return true;
    }
    return false;
}

// Render a text label into a fresh texture and wrap it in a sprite.
// The texture is returned through out_tex so the caller can free it.
static bsglSprite* CreateLabel(bsglFont* font, char const* text,
                               int w, int h, HTEXTURE* out_tex) {
    *out_tex = bsgl->Texture_Create(w, h);
    // clear to fully transparent pixels
    int tw = bsgl->Texture_GetWidth(*out_tex);
    int th = bsgl->Texture_GetHeight(*out_tex);
    DWORD* pixels = bsgl->Texture_CreateData(tw, th);
    memset(pixels, 0, tw*th*sizeof(DWORD));
    bsgl->Texture_Update(*out_tex, pixels, 0, 0, tw, th);
    bsgl->Texture_FreeData(pixels);

    // draw the text roughly centered (baseline at ~2/3 of the height)
    font->BeginDrawTexture(*out_tex, w/6, (h*2)/3, h);
    for( char const* p = text; *p; ++p ) {
        font->DrawGlyph((wchar_t)*p);
    }
    font->EndDrawTexture();

    bsglSprite* spr = new bsglSprite(*out_tex, 0, 0, (float)w, (float)h);
    // the glyphs are white; tint them dark navy for contrast
    spr->SetColor(RGBA(0x10, 0x10, 0x30, 0xFF));
    return spr;
}

bool LogicFunc() {
    bsgl->Control_GetState();

    if( bsgl->Control_IsDown(INP_ESC) || quit_requested ) {
        return true;
    }

    // Translate the BSGL mouse state into bsglWidget events.
    // The widget coordinates of kids are relative to their parent,
    // so the panel is fed with plain screen coordinates.
    float mx = (float)bsgl->Control_GetMouseX();
    float my = (float)bsgl->Control_GetMouseY();

    MouseState state;
    if( bsgl->Control_IsDown(INP_MOUSEL) ) {
        state = MouseState_Down;
    }else if( bsgl->Control_IsUp(INP_MOUSEL) ) {
        state = MouseState_Up;
    }else if( bsgl->Control_IsPassing(INP_MOUSEL) ) {
        state = MouseState_Passing;
    }else {
        state = MouseState_Default;
    }
    panel->MouseAt(mx, my, state);

    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();
    bsgl->Gfx_Clear(bg_colors[bg_index]);

    // Render the panel; it draws itself and all of its kids
    panel->Render(0, 0);

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial06.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 06 - Creating a simple menu");
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, 800);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, 600);

    if( bsgl->System_Initiate() ) {
        // Labels need a font; without one the menu still works
        bsglFont* font = 0;
        if( FontFileExists() ) {
            bsgl->System_Log("Using font file: %s", FONT_FILE);
            font = new bsglFont(FONT_FILE, 22);
        }else {
            bsgl->System_Log("Warning: %s not found, buttons will have no labels.", FONT_FILE);
        }

        // The panel: a container widget with its own background
        panel = new bsglWidget(250, 150, 300, 300);
        panel->SetBackgroundColor(RGBA(0x30, 0x30, 0x50, 0xFF));

        // Three labelled buttons, positioned relative to the panel
        HTEXTURE label_tex[3] = { 0, 0, 0 };
        Button* buttons[3];
        char const* captions[3] = { "Color", "Log", "Quit" };
        for( int i=0; i<3; ++i ) {
            bsglSprite* label = font ? CreateLabel(font, captions[i], 200, 60, &label_tex[i]) : 0;
            buttons[i] = new Button(50, 40 + i*80, 200, 60, i+1, label);
            panel->AddKid(buttons[i]);
        }

        bsgl->System_Start();

        for( int i=0; i<3; ++i ) {
            delete buttons[i];
            if( label_tex[i] ) {
                bsgl->Texture_Free(label_tex[i]);
            }
        }
        delete panel;
        delete font;

        bsgl->System_Shutdown();
    }else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
