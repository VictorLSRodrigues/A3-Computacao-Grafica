
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
    # Application: Sandbox
    src/applications/sandbox/main.cpp
    src/applications/sandbox/application.cpp
    src/applications/sandbox/world.cpp

    # Application: Sandbox UI
    src/applications/sandbox/ui/HUD.cpp
    src/applications/sandbox/ui/environment_info.cpp
    src/applications/sandbox/ui/light_panel.cpp
    src/applications/sandbox/ui/menu.cpp

    # Engine: Editor
    src/engine/editor/editor.cpp
    src/engine/editor/gizmo.cpp

    # Engine: Platform
    src/engine/platform/window.cpp

    # Engine: Graphics Buffers
    src/engine/graphics/buffers/EBO.cpp
    src/engine/graphics/buffers/VAO.cpp
    src/engine/graphics/buffers/VBO.cpp

    # Engine: Graphics Camera
    src/engine/graphics/camera/camera.cpp

    # Engine: Graphics Geometry
    src/engine/graphics/geometry/bounding_box.cpp
    src/engine/graphics/geometry/geometry.cpp
    src/engine/graphics/geometry/shapes.cpp

    # Engine: Graphics Mesh
    src/engine/graphics/mesh/mesh.cpp

    # Engine: Graphics Model
    src/engine/graphics/model/model.cpp
    src/engine/graphics/model/model_loader.cpp

    # Engine: Graphics Resources
    src/engine/graphics/resources/cubemap.cpp
    src/engine/graphics/resources/shader.cpp
    src/engine/graphics/resources/texture.cpp

    # Engine: Graphics Environment
    src/engine/graphics/environment/skybox.cpp

    # Engine: Graphics Manager
    src/engine/graphics/resources/texture_manager.cpp

    # Engine: Renderer
    src/engine/renderer/renderer.cpp

    # Engine: Scene
    src/engine/scene/light.cpp
    src/engine/scene/picking.cpp
    src/engine/scene/scene.cpp
    src/engine/scene/scenarios/rain_scenario.cpp

    # Engine: UI
    src/engine/ui/UI.cpp
    src/engine/ui/font.cpp

    # Dependencies
    Libraries/glad/src/glad.c
)

add_subdirectory(external/doomgeneric/doomgeneric)

target_include_directories(OpenGLProjeto PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external"
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(OpenGLProjeto PRIVATE
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
)

find_package(OpenGL REQUIRED)

target_link_libraries(OpenGLProjeto PRIVATE
    glfw
    OpenGL::GL
)
