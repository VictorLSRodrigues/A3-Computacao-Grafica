cmake_minimum_required(VERSION 3.20)

project(OpenGLProjeto
    VERSION 1.0.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(Libraries/glfw-3.5.1)

add_executable(OpenGLProjeto
    src/main.cpp
    src/shader.cpp
    src/mesh.cpp
    src/formas.cpp
    src/window.cpp
    Libraries/glad/src/glad.c
)

target_include_directories(OpenGLProjeto PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include"
)

target_compile_definitions(OpenGLProjeto PRIVATE
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
)

find_package(OpenGL REQUIRED)

target_link_libraries(OpenGLProjeto PRIVATE
    glfw
    OpenGL::GL
)