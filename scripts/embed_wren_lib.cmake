#=====================================================================
# embed_wren_lib.cmake - embed src/advance/bsgl.wren as a C string
#
# Invoked as: cmake -D SRC=<in> -D DST=<out> -P embed_wren_lib.cmake
# Generates a C header with the Wren source in a raw string literal,
# included by src/advance/bsglwren.cpp (like wren_opt_meta.wren.inc).
#=====================================================================

file(READ "${SRC}" CONTENT)
file(WRITE "${DST}"
"// generated from src/advance/bsgl.wren by scripts/embed_wren_lib.cmake - do not edit
static const char* BSGL_WREN_LIB_SOURCE =
R\"wren(${CONTENT})wren\";
")
