/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core system functions
*/

#include "bsgl_impl.h"

#include <QApplication>
#include <QTimer>

#include "MainWindow.h"
#include "MainTask.h"
#include "ScreenWidget.h"

#define _KEY_BUF_SIZE 256

QApplication* a;

bool isRunning = false;

int         nRef = 0;
BSGL_Impl*   pBSGL = 0;
static bool  bInit = false;

BSGL* CALL bsglCreate(int ver) {
    if( BSGL_VERSION == ver ) {
        return (BSGL*)BSGL_Impl::_Interface_Get();
    } else {
        return 0;
    }
}

BSGL_Impl* BSGL_Impl::_Interface_Get() {
    if( 0 == pBSGL ) {
        pBSGL = new BSGL_Impl();
    }
    ++nRef;
    return pBSGL;
}

void CALL BSGL_Impl::Release() {
    --nRef;
    if( 0 == nRef) {
        delete pBSGL;
        pBSGL = 0;
    }
}

bool CALL BSGL_Impl::System_Initiate() {
    System_Log("BSGL Started...\n");
    System_Log("BSGL version: %X.%X", BSGL_VERSION>>8, BSGL_VERSION&0xFF);
    System_Log("Application: %s\n", szTitle);

    _LoadConfig("config.xml");

    if( !_GfxInit() ) {
        _PostError("Can't initialize OpenGL.");
        System_Shutdown();
        return false;
    }

    fTime = 0.0f;

    bInit = true;

    return true;
}

void CALL BSGL_Impl::System_Shutdown() {
    System_Log("\nFinishing...");
    _GfxDone();

    System_Log("The End.");
}

bool CALL BSGL_Impl::System_Start() {
    QTime clock;
    clock.start();
    float time = clock.elapsed() / 1000.0f;
    float old_time = time;
    float old_logic_time = time;
    float old_render_time = time;
    float time4fps = time;
    float logic_dt = 0.0f;
    float render_dt = 0.0f;
    unsigned int fps = 0;

    while( isRunning ) {
        a->processEvents();

        time = clock.elapsed() / 1000.0f;
        float _t = time - old_time;
        old_time = time;
        fTime += _t;
        if( !bActive ) {
            old_logic_time += _t;
            old_render_time += _t;
            QThread::usleep(1);
            continue;
        }

        if( ( 0 == nLFPS )
         || ( (time-old_logic_time) >= (1.0f/nLFPS) ) ) {
            fDeltaTime = logic_dt;
            if( fpLogicFunc != 0 ) {
                if( fpLogicFunc() ) {
                    break;
                }
            }
            //fLogicDeltaTime
            logic_dt = ( time - old_logic_time );
            if( nLFPS != 0 ) {
                old_logic_time += (1.0f/nLFPS);
            }else {
                old_logic_time = time;
            }
        }

        if( ( 0 == nRFPS )
         || ( (time-old_render_time) >= (1.0f/nRFPS) ) ) {
            fDeltaTime = render_dt;
            if( fpRenderFunc != 0 ) {
                if( fpRenderFunc() ) {
                    break;
                }
                ++fps;
            }
            //glFlush();
            ScreenWidget::instance()->updateGL();

            //fRenderDeltaTime
            render_dt = ( time - old_render_time );
            if( nRFPS != 0 ) {
                old_render_time += (1.0f/nRFPS);
            }else {
                old_render_time = time;
            }
        }

        if( time-time4fps >= 1.0f ) {
            //System_Log("fps: %u\ntime: %f", fps, Timer_GetTime());
            nFPS = fps;
            fps = 0;
            time4fps += 1.0f;
        }

        QThread::usleep(1);
    }
    return true;
}

void CALL BSGL_Impl::System_SetStateBool(bsglBoolState state, bool value) {
    switch( state ) {
        case BSGL_WINDOWED:
            bWindowed = value;
            break;
        default:
            break;
    }
}

void CALL BSGL_Impl::System_SetStateFunc(bsglFuncState state, bsglCallback value) {
    switch( state ) {
        case BSGL_LOGICFUNC:
            fpLogicFunc = value;
            break;
        case BSGL_RENDERFUNC:
            fpRenderFunc = value;
            break;
        //case BSGL_INPUTFUNC:
        //    procInputFunc = value;
        //    break;
        default:
            break;
    }
}

void CALL BSGL_Impl::System_SetStateInt(bsglIntState state, int value) {
    switch( state ) {
        case BSGL_SCREENWIDTH:
            nScreenWidth = value;
            break;
        case BSGL_SCREENHEIGHT:
            nScreenHeight = value;
            break;
        case BSGL_NUMOFLFPS:
            nLFPS = value;
            break;
        case BSGL_NUMOFRFPS:
            nRFPS = value;
            break;
        case BSGL_POLYMODE:
            nPolyMode = value;
            break;
        default:
            break;
    }
}

int CALL BSGL_Impl::System_GetStateInt(bsglIntState state) {
    switch( state ) {
        case BSGL_SCREENWIDTH:
            return nScreenWidth;
            break;
        case BSGL_SCREENHEIGHT:
            return nScreenHeight;
            break;
        case BSGL_NUMOFLFPS:
            return nLFPS;
            break;
        case BSGL_NUMOFRFPS:
            return nRFPS;
            break;
        case BSGL_POLYMODE:
            return nPolyMode;
            break;
        default:
            break;
    }
    return 0;
}

void CALL BSGL_Impl::System_SetStateString(bsglStringState state, const char* value) {
    switch( state ) {
        case BSGL_TITLE:
            if( value != 0 ) {
                strcpy(szTitle, value);
            }else {
                szTitle[0] = (char)0;
            }
            break;
        case BSGL_CFGFILE:
            if( value != 0 ) {
                strcpy(szCfgFile, value);
            }else {
                szCfgFile[0] = (char)0;
            }
            break;
        case BSGL_LOGFILE:
            if( value != 0 ) {
                QFile file(szLogFile);
                strcpy(szLogFile, value);
                if (!file.isWritable()) {
                    szLogFile[0] = (char)0;
                }
            }else {
                szLogFile[0] = (char)0;
            }
            break;
        default:
            break;
    }
}

char* CALL BSGL_Impl::System_GetErrorMessage() {
    return szError;
}

void CALL BSGL_Impl::System_Log(const char *szFormat, ...) {
    va_list vl;

    if(!szLogFile[0]) {
        return;
    }

    QFile file(szLogFile);
    if (!file.open(QIODevice::Append)) {
        return;
    }

    char buffer[1024];

    va_start(vl, szFormat);
    int n = vsnprintf(buffer, 1023, szFormat, vl);
    buffer[n] = 0;
    file.write(buffer);
    va_end(vl);
    va_start(vl, szFormat);
    vfprintf(stdout, szFormat, vl);
    va_end(vl);

    file.write("\n");
    fprintf(stdout, "\n");
}

BSGL_Impl::BSGL_Impl() {
    bWindowed = false;
    nScreenWidth = 800;
    nScreenHeight = 600;
    nScreenBPP = 32;
    szTitle[0] = (char)0;
    szError[0] = (char)0;
    szLogFile[0] = (char)0;
    bActive = true;
    nLFPS = 60;
    nRFPS = 0;
    fpLogicFunc     = 0;
    fpRenderFunc    = 0;
    fTime = 0.0f;
    fDeltaTime = 0.0f;
    nFPS = 0;
    nPolyMode = 0;

    this->_key_buf = new unsigned int[_KEY_BUF_SIZE];
    memset(_key_buf, 0, _KEY_BUF_SIZE*sizeof(unsigned int));

    memset(qtKeyStates, 0, 0xFF*sizeof(bool));
    qtLeftButton = qtRightButton = false;
}

void CALL BSGL_Impl::_LoadConfig(char const* filename) {
    if( !filename[0] ) {
        return;
    }
    System_SetStateString(BSGL_CFGFILE, filename);
    bool windowed = Config_GetInt("Screen", "Windowed", bWindowed?1:0)==1 ? true : false;
    System_SetStateBool(BSGL_WINDOWED, windowed);
    int width   = Config_GetInt("Screen", "Width", nScreenWidth);
    int height  = Config_GetInt("Screen", "Height", nScreenHeight);
    System_SetStateInt(BSGL_SCREENWIDTH, width);
    System_SetStateInt(BSGL_SCREENHEIGHT, height);
    int logic_fps   = Config_GetInt("FPS", "Logic", nLFPS);
    int render_fps  = Config_GetInt("FPS", "Render", nRFPS);
    System_SetStateInt(BSGL_NUMOFLFPS, logic_fps);
    System_SetStateInt(BSGL_NUMOFRFPS, render_fps);
    char title[512];
    strcpy(title, Config_GetString("Window", "Title", szTitle[0]?szTitle:"BSGL GAME"));
    System_SetStateString(BSGL_TITLE, title);
}

void BSGL_Impl::_PostError(const char* error, ...) {
    va_list vl;
    char _error[256];

    va_start(vl, error);
    vsprintf(_error, error, vl);
    va_end(vl);

    System_Log("Error: %s", _error);
    strcpy(szError, _error);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    a = &app;

    MainWindow w;
    w.show();

    MainTask task(a);
    QObject::connect(a, SIGNAL(lastWindowClosed()), &task, SLOT(quit()));
    QObject::connect(&task, SIGNAL(finished()), a, SLOT(quit()));
    QTimer::singleShot(0, &task, SLOT(run()));

    int ret = app.exec();
    return ret;
}
