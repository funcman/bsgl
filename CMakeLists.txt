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
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/freetype/CMakeLists.txt"
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/spine-runtimes/spine-cpp/CMakeLists.txt"
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/DragonBonesCPP/DragonBones/src"
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/zlib/CMakeLists.txt"
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/libspng/spng/spng.c"
OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/libjpeg-turbo/CMakeLists.txt")
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

# tinyxml2
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(3rd/tinyxml2 EXCLUDE_FROM_ALL)

# spine-cpp (from spine-runtimes, branch 4.2): static library, built
# directly from sources - the bundled CMakeLists pulls in example
# dependencies (glfw etc.) we do not want
file(GLOB SPINE_CPP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/spine-runtimes/spine-cpp/spine-cpp/src/spine/*.cpp")
add_library(spine-cpp STATIC ${SPINE_CPP_SOURCES})
target_include_directories(spine-cpp
    PUBLIC 3rd/spine-runtimes/spine-cpp/spine-cpp/include)
set_target_properties(spine-cpp PROPERTIES CXX_STANDARD 14)

# DragonBonesCPP: ships no CMake build of its own. The build scripts
# copy scripts/DragonBonesCPP-CMakeLists.txt in as its CMakeLists.txt;
# do the same here so that invoking cmake directly also works.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/DragonBonesCPP/CMakeLists.txt")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/DragonBonesCPP-CMakeLists.txt"
                   "${CMAKE_CURRENT_SOURCE_DIR}/3rd/DragonBonesCPP/CMakeLists.txt" COPYONLY)
endif()
# no EXCLUDE_FROM_ALL: bsgl links dragonbones (bsgldbones.cpp)
add_subdirectory(3rd/DragonBonesCPP)

# zlib (required by libspng)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(3rd/zlib EXCLUDE_FROM_ALL)

# libspng: single-file PNG decoder, built directly - its CMakeLists
# expects a system zlib via find_package, we use our own submodule
add_library(spng STATIC 3rd/libspng/spng/spng.c)
target_compile_definitions(spng PUBLIC SPNG_STATIC)
target_include_directories(spng PUBLIC 3rd/libspng/spng)
target_link_libraries(spng PUBLIC zlibstatic)

# libjpeg-turbo: static only, no SIMD (NASM is not available here).
# Its own CMakeLists refuses add_subdirectory() integration, so the
# library is built directly from sources with generated config headers.
set(LJT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rd/libjpeg-turbo")
set(LJT_GEN "${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo-gen")
file(MAKE_DIRECTORY "${LJT_GEN}")

# values for the jconfig/jconfigint/jversion templates (libjpeg-turbo 3.2.1, no SIMD)
set(VERSION 3.2.1)
set(LIBJPEG_TURBO_VERSION_NUMBER 3002001)
set(JPEG_LIB_VERSION 62)
set(COPYRIGHT_YEAR "1991-2026")
string(TIMESTAMP BUILD "%Y%m%d")
set(PACKAGE_NAME "libjpeg-turbo")
set(HIDDEN "")
# MSVC-specific keywords vs. their portable C11/GNU equivalents
if(MSVC)
    set(INLINE "__inline")
    set(THREAD_LOCAL "__declspec(thread)")
    # <intrin.h> is MSVC-only; AppleClang 21+ ships a stub that fails
    # on #include_next, so only enable it for real MSVC builds
    set(HAVE_INTRIN_H 1)
else()
    set(INLINE "inline")
    set(THREAD_LOCAL "_Thread_local")
endif()
set(SIZE_T 8)
set(SIZEOF_SIZE_T 8)
set(SIMD_ARCHITECTURE NONE)
set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
configure_file("${LJT_DIR}/src/jconfig.h.in"     "${LJT_GEN}/jconfig.h")
configure_file("${LJT_DIR}/src/jconfigint.h.in"  "${LJT_GEN}/jconfigint.h")
configure_file("${LJT_DIR}/src/jversion.h.in"    "${LJT_GEN}/jversion.h")

file(GLOB LJT_WRAPPER_SOURCES "${LJT_DIR}/src/wrapper/*.c")
# template.c is the generator template for the wrapper files, not a source
list(REMOVE_ITEM LJT_WRAPPER_SOURCES "${LJT_DIR}/src/wrapper/template.c")
add_library(turbojpeg-static STATIC
    ${LJT_WRAPPER_SOURCES}
    ${LJT_DIR}/src/jcapimin.c ${LJT_DIR}/src/jchuff.c   ${LJT_DIR}/src/jcicc.c
    ${LJT_DIR}/src/jcinit.c   ${LJT_DIR}/src/jclhuff.c  ${LJT_DIR}/src/jcmarker.c
    ${LJT_DIR}/src/jcmaster.c ${LJT_DIR}/src/jcomapi.c  ${LJT_DIR}/src/jcparam.c
    ${LJT_DIR}/src/jcphuff.c  ${LJT_DIR}/src/jctrans.c  ${LJT_DIR}/src/jdapimin.c
    ${LJT_DIR}/src/jdatadst.c ${LJT_DIR}/src/jdatasrc.c ${LJT_DIR}/src/jdhuff.c
    ${LJT_DIR}/src/jdicc.c    ${LJT_DIR}/src/jdinput.c  ${LJT_DIR}/src/jdlhuff.c
    ${LJT_DIR}/src/jdmarker.c ${LJT_DIR}/src/jdmaster.c ${LJT_DIR}/src/jdphuff.c
    ${LJT_DIR}/src/jdtrans.c  ${LJT_DIR}/src/jerror.c   ${LJT_DIR}/src/jfdctflt.c
    ${LJT_DIR}/src/jmemmgr.c  ${LJT_DIR}/src/jmemnobs.c ${LJT_DIR}/src/jpeg_nbits.c
    ${LJT_DIR}/src/jaricom.c  ${LJT_DIR}/src/jcarith.c  ${LJT_DIR}/src/jdarith.c
    ${LJT_DIR}/src/turbojpeg.c ${LJT_DIR}/src/transupp.c
    ${LJT_DIR}/src/jdatadst-tj.c ${LJT_DIR}/src/jdatasrc-tj.c
    ${LJT_DIR}/src/rdbmp.c ${LJT_DIR}/src/wrbmp.c)
target_compile_definitions(turbojpeg-static PRIVATE
    BMP_SUPPORTED PNG_SUPPORTED PPM_SUPPORTED SPNG_STATIC)
target_link_libraries(turbojpeg-static PRIVATE spng)
target_include_directories(turbojpeg-static
    PUBLIC  "${LJT_DIR}/src"
    PRIVATE "${LJT_GEN}")
unset(VERSION)
unset(BUILD)
unset(PACKAGE_NAME)

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
    src/advance/bsglspine.cpp
    src/advance/bsgldbones.cpp
    src/gui/bsglfont.cpp
    src/gui/bsglwidget.cpp
)

target_include_directories(bsgl
    PUBLIC
        include
    PRIVATE
        src/core
)

# freetype is PUBLIC: include/bsglfont.h includes <ft2build.h>
# spine-cpp is PUBLIC: bsglSpine forwards spine types to its users
# dragonbones is PUBLIC: bsglDBones forwards dragonBones types to its users
target_link_libraries(bsgl PUBLIC SDL3::SDL3 freetype tinyxml2::tinyxml2 spine-cpp dragonbones)

# image decoders used by graphics.cpp (Texture_Load)
target_link_libraries(bsgl PRIVATE spng turbojpeg-static)

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

#--- Tutorials -----------------------------------------------------------
option(BSGL_BUILD_TUTORIALS "Build the BSGL tutorials" ON)
if(BSGL_BUILD_TUTORIALS)
    add_subdirectory(tutorials)
endif()
