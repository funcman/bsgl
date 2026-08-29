/*
** BSGL Tutorial 06 - Creating a simple menu with bsglGUIWidget
** (adapted from hge_tut06 - Creating menus)
**
** bsglGUIWidget is a minimal GUI building block: a colored rectangle
** with a position, a list of child widgets, mouse hit-testing and
** four overridable callbacks (OnRender/OnOver/OnDown/OnUp).
** Children use coordinates relative to their parent.
**
** This tutorial builds a panel with three labelled buttons:
**   - "Color" cycles the background color,
**   - "Log"   writes a line to the log,
**   - "Quit"  quits the application.
**
** The button labels are text meshes rendered through the bsglFont
** glyph-atlas cache (bsglTextMesh, built once, rendered every frame),
** using the bundled DejaVu Sans font (tutorials/res/font.ttf).
** Without the font the buttons simply show no text.
** Pressing ESC also quits.
*/

#include "bsgl.h"
#include "bsglfont.h"
#include "bsgltextmesh.h"
#include "bsglguiwidget.h"
#include <stdio.h>
#include <string.h>

static BSGL* bsgl = nullptr;

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
class Button : public bsglGUIWidget {
public:
    Button(int x, int y, int w, int h, int id, bsglTextMesh* label)
    :   bsglGUIWidget(x, y, w, h)
    ,   id_(id)
    ,   label_(label) {
        SetBackgroundColor(RGBA(0x70, 0x70, 0x90, 0xFF));
    }

    ~Button() {
        delete label_;
    }

    void OnRender(float x, float y) {
        // draw the background quad first, the label on top
        bsglGUIWidget::OnRender(x, y);
        if (label_) {
            // center the label inside the button
            label_->Render(x_+x+(w_-label_->GetWidth())/2,
                           y_+y+(h_-label_->GetHeight())/2,
                           RGBA(0x10, 0x10, 0x30, 0xFF));
        }
    }

    void OnOver(float x, float y) {
        if (TestAt(x, y)) {
            SetBackgroundColor(RGBA(0xB0, 0xB0, 0xD0, 0xFF));
        } else {
            SetBackgroundColor(RGBA(0x70, 0x70, 0x90, 0xFF));
        }
    }

    void OnDown() {
        SetBackgroundColor(RGBA(0x50, 0x50, 0x70, 0xFF));
    }

    void OnUp(bool inside) {
        if (!inside) {
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
    int             id_;
    bsglTextMesh*   label_;
};

static bsglGUIWidget* panel = nullptr;

// The bundled DejaVu Sans font (tutorials/res/font.ttf), copied next
// to the executable at build time - see tutorials/CMakeLists.txt.
static const char* const FONT_FILE = "font.ttf";

static bool FontFileExists() {
    FILE* f = fopen(FONT_FILE, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// Build a reusable label mesh from the font's glyph-atlas cache.
static bsglTextMesh* CreateLabel(bsglFont* font, char const* text) {
    bsglTextMesh* mesh = new bsglTextMesh();
    font->RenderText(mesh, text);
    return mesh;
}

bool LogicFunc() {
    bsgl->Control_GetState();

    if (bsgl->Control_IsDown(INP_ESC) || quit_requested) {
        return true;
    }

    // Translate the BSGL mouse state into bsglGUIWidget events.
    // The widget coordinates of kids are relative to their parent,
    // so the panel is fed with plain screen coordinates.
    float mx = (float)bsgl->Control_GetMouseX();
    float my = (float)bsgl->Control_GetMouseY();

    MouseState state;
    if (bsgl->Control_IsDown(INP_MOUSEL)) {
        state = MouseState_Down;
    } else if (bsgl->Control_IsUp(INP_MOUSEL)) {
        state = MouseState_Up;
    } else if (bsgl->Control_IsPassing(INP_MOUSEL)) {
        state = MouseState_Passing;
    } else {
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

    if (bsgl->System_Initiate()) {
        // Labels need a font; without one the menu still works
        bsglFont* font = nullptr;
        if (FontFileExists()) {
            bsgl->System_Log("Using font file: %s", FONT_FILE);
            font = new bsglFont(FONT_FILE, 22);
        } else {
            bsgl->System_Log("Warning: %s not found, buttons will have no labels.", FONT_FILE);
        }

        // The panel: a container widget with its own background
        panel = new bsglGUIWidget(250, 150, 300, 300);
        panel->SetBackgroundColor(RGBA(0x30, 0x30, 0x50, 0xFF));

        // Three labelled buttons, positioned relative to the panel
        Button* buttons[3];
        char const* captions[3] = { "Color", "Log", "Quit" };
        for( int i=0; i<3; ++i ) {
            bsglTextMesh* label = font ? CreateLabel(font, captions[i]) : nullptr;
            buttons[i] = new Button(50, 40 + i*80, 200, 60, i+1, label);
            panel->AddKid(buttons[i]);
        }

        bsgl->System_Start();

        for( int i=0; i<3; ++i ) {
            delete buttons[i];
        }
        delete panel;
        delete font;

        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
