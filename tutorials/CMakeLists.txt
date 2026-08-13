#=====================================================================
# BSGL tutorials - adapted from the HGE 1.9 tutorial series
#
# Each tutorial is a single-file executable linked against the bsgl
# static library. The library provides the platform entry point and
# calls bsgl_main(), so a tutorial only has to define bsgl_main().
#=====================================================================

set(BSGL_TUTORIALS
    tutorial01
    tutorial02
    tutorial03
    tutorial04
    tutorial05
    tutorial06
    tutorial07
)

# whatever resources are present in res/ are copied next to the
# executables (globbed at configure time, so re-run the configure
# step after adding a file, e.g. the zazaka.bmp for tutorial07)
file(GLOB BSGL_TUT_RES "${CMAKE_CURRENT_SOURCE_DIR}/res/*.bmp"
                       "${CMAKE_CURRENT_SOURCE_DIR}/res/*.ttf")

foreach(tut ${BSGL_TUTORIALS})
    add_executable(${tut} ${tut}/main.cpp)
    target_link_libraries(${tut} PRIVATE bsgl)

    if(BSGL_TUT_RES)
        add_custom_command(TARGET ${tut} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${BSGL_TUT_RES}
                "$<TARGET_FILE_DIR:${tut}>"
        )
    endif()

    # run/debug from the executable directory inside Visual Studio,
    # so that the resources are found
    set_target_properties(${tut} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${tut}>")
endforeach()
