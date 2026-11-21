file(GLOB DOOM_GENERIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

list(FILTER DOOM_GENERIC_SOURCES EXCLUDE REGEX
    "/(doomgeneric_(allegro|emscripten|linuxvt|sdl|soso|sosox|xlib)|gusconf|i_allegromusic|i_allegrosound|i_sdlmusic|i_sdlsound|icon|mus2mid)\\.c$")

set(DOOM_GENERIC_PLATFORM_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/doomgeneric_win.c")
set(DOOM_GENERIC_CORE_SOURCES ${DOOM_GENERIC_SOURCES})
list(REMOVE_ITEM DOOM_GENERIC_CORE_SOURCES "${DOOM_GENERIC_PLATFORM_SOURCE}")

add_library(DoomCore STATIC ${DOOM_GENERIC_CORE_SOURCES})

target_include_directories(DoomCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(DoomCore PRIVATE
    SNDSERV
    _DEFAULT_SOURCE
)

target_link_libraries(DoomCore PUBLIC
    m
    user32
)

add_executable(DoomGeneric "${DOOM_GENERIC_PLATFORM_SOURCE}")

set_target_properties(DoomGeneric PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(DoomGeneric PRIVATE
    DoomCore
    gdi32
)
