#=====================================================================
# BSGL - Buggy-Mushroom's Simple Game Library
# SDL3 frontend, CMake build
#=====================================================================
cmake_minimum_required(VERSION 3.16)

project(bsgl LANGUAGES C CXX)

#--- Output layout: build/bin/<Config>, build/lib/<Config> -------------
# (must precede any add_subdirectory/add_library to take effect)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

#--- Third-party dependencies (git submodules under 3rd/) --------------
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/SDL3/CMakeLists.txt"
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/freetype/CMakeLists.txt")
    message(FATAL_ERROR
        "git submodules are missing. Run:\n"
        "  git submodule update --init --recursive")
endif()

# SDL3: shared library only, no tests
set(SDL_SHARED        ON  CACHE BOOL "" FORCE)
set(SDL_STATIC        OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY  OFF CACHE BOOL "" FORCE)
add_subdirectory(3rd/SDL3 EXCLUDE_FROM_ALL)

# freetype: static library, minimal feature set (keep the build lean)
set(FT_DISABLE_ZLIB    TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2   TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG     TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI  TRUE CACHE BOOL "" FORCE)
add_subdirectory(3rd/freetype EXCLUDE_FROM_ALL)

#--- bsgl static library -------------------------------------------------
add_library(bsgl STATIC
    src/core/system.cpp
    src/core/config.cpp
    src/core/graphics.cpp
    src/core/control.cpp
    src/core/timer.cpp
    src/core/random.cpp
    src/advance/bsglrect.cpp
    src/advance/bsglsprite.cpp
    src/advance/bsglanim.cpp
    src/gui/bsglfont.cpp
    src/gui/bsglwidget.cpp
    src/dependencies/tinyxml/tinystr.cpp
    src/dependencies/tinyxml/tinyxmlparser.cpp
    src/dependencies/tinyxml/tinyxmlerror.cpp
    src/dependencies/tinyxml/tinyxml.cpp
)

target_include_directories(bsgl
    PUBLIC
        include
    PRIVATE
        src/core
        src/dependencies/tinyxml
)

# freetype is PUBLIC: include/bsglfont.h includes <ft2build.h>
target_link_libraries(bsgl PUBLIC SDL3::SDL3 freetype)

# OpenGL
if(WIN32)
    target_link_libraries(bsgl PUBLIC opengl32)
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(bsgl PUBLIC ${OPENGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(bsgl PUBLIC OpenGL::GL)
endif()
