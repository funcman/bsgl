QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET              =   bsgl
TEMPLATE            =   lib
CONFIG              +=  staticlib

INCLUDEPATH         +=                              \
    include                                         \
    src/core                                        \
    src/qt                                          \
    src/dependencies/tinyxml

ios {
    INCLUDEPATH     +=                              \
        3rd_parties/freetype2/include/ios/freetype2
    QMAKE_LIBDIR    +=                              \
        $$PWD/3rd_parties/freetype2/prebuilt/ios
    LIBS            +=                              \
        -lfreetype
}

macx {
    INCLUDEPATH     +=                              \
        3rd_parties/freetype2/include/mac/freetype2 \
        3rd_parties/zlib/include
    QMAKE_LIBDIR    +=                              \
        $$PWD/3rd_parties/freetype2/prebuilt/mac    \
        $$PWD/3rd_parties/zlib/prebuilt/mac
    LIBS            +=                              \
        -lfreetype                                  \
        -lz
}

HEADERS             +=                              \
    include/bsgl.h                                  \
    include/bsglrect.h                              \
    include/bsglsprite.h                            \
    include/bsglanim.h                              \
    include/bsglfont.h                              \
    include/bsglwidget.h                            \
    src/core/bsgl_impl.h                            \
    src/qt/MainTask.h                               \
    src/qt/MainWindow.h                             \
    src/qt/ScreenWidget.h                           \
    src/dependencies/tinyxml/tinystr.h              \
    src/dependencies/tinyxml/tinyxml.h

SOURCES         +=                                  \
    src/core/system.cpp                             \
    src/core/config.cpp                             \
    src/core/graphics.cpp                           \
    src/core/control.cpp                            \
    src/core/timer.cpp                              \
    src/core/random.cpp                             \
    src/advance/bsglrect.cpp                        \
    src/advance/bsglsprite.cpp                      \
    src/advance/bsglanim.cpp                        \
    src/gui/bsglfont.cpp                            \
    src/gui/bsglwidget.cpp                          \
    src/qt/MainTask.cpp                             \
    src/qt/MainWindow.cpp                           \
    src/qt/ScreenWidget.cpp                         \
    src/dependencies/tinyxml/tinystr.cpp            \
    src/dependencies/tinyxml/tinyxmlparser.cpp      \
    src/dependencies/tinyxml/tinyxmlerror.cpp       \
    src/dependencies/tinyxml/tinyxml.cpp
