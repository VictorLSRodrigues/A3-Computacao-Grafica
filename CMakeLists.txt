
cmake_minimum_required(VERSION 3.20)

project(OpenGLProjeto LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_DIR "${CMAKE_SOURCE_DIR}/Libraries/glfw-3.5.1")
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/Libraries/glad")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Libraries/include")

add_executable(OpenGLProjeto
    src/main.cpp
    Libraries/glad/src/glad.c
)

target_include_directories(OpenGLProjeto PRIVATE
    "${INCLUDE_DIR}"
    "${GLFW_DIR}/include"
)

target_link_libraries(OpenGLProjeto PRIVATE
    "${CMAKE_SOURCE_DIR}/Libraries/lib/libglfw3.a"
    opengl32
    gdi32
)

if(WIN32)
    target_link_libraries(OpenGLProjeto PRIVATE user32 shell32)
endif()
