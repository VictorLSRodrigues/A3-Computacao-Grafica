file(GLOB DOOM_GENERIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

list(FILTER DOOM_GENERIC_SOURCES EXCLUDE REGEX
    "/(doomgeneric_(allegro|emscripten|linuxvt|sdl|soso|sosox|xlib)|gusconf|i_allegromusic|i_allegrosound|i_sdlmusic|i_sdlsound|icon|mus2mid)\\.c$")

add_executable(DoomGeneric ${DOOM_GENERIC_SOURCES})

set_target_properties(DoomGeneric PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_definitions(DoomGeneric PRIVATE
    SNDSERV
    _DEFAULT_SOURCE
)

target_link_libraries(DoomGeneric PRIVATE
    m
    gdi32
    user32
)
