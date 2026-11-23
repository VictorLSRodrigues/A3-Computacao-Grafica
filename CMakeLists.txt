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
add_subdirectory(external/doomgeneric/doomgeneric)

find_package(OpenGL REQUIRED)

# Código compartilhado do motor. Cada aplicação possui seu próprio executável,
# mas utiliza a mesma implementação do Rotor Engine.
set(ROTOR_ENGINE_SOURCES
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
    src/engine/graphics/resources/texture_manager.cpp

    # Engine: Graphics Environment
    src/engine/graphics/environment/skybox.cpp

    # Engine: Renderer
    src/engine/renderer/rain_renderer.cpp
    src/engine/renderer/renderer.cpp

    # Engine: Physics
    src/engine/physics/physics_world.cpp

    # Engine: Scene
    src/engine/scene/light.cpp
    src/engine/scene/picking.cpp
    src/engine/scene/scene.cpp
    src/engine/scene/scene_persistence.cpp
    src/engine/scene/scenarios/rain_scenario.cpp

    # Engine: UI
    src/engine/ui/UI.cpp
    src/engine/ui/font.cpp

    # Dependency
    Libraries/glad/src/glad.c
)

add_library(rotor_engine STATIC ${ROTOR_ENGINE_SOURCES})

target_include_directories(rotor_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external"
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(rotor_engine PUBLIC
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
)

target_link_libraries(rotor_engine PUBLIC
    glfw
    OpenGL::GL
)

# Aplicação existente. O nome do alvo OpenGLProjeto é preservado para manter
# compatibilidade com os comandos de build já utilizados no projeto.
add_executable(OpenGLProjeto
    src/applications/sandbox/main.cpp
    src/applications/sandbox/application.cpp
    src/applications/sandbox/world.cpp
    src/applications/sandbox/doom/doom_game.cpp

    # Sandbox UI
    src/applications/sandbox/ui/HUD.cpp
    src/applications/sandbox/ui/environment_info.cpp
    src/applications/sandbox/ui/light_panel.cpp
    src/applications/sandbox/ui/menu.cpp
)

target_compile_definitions(OpenGLProjeto PRIVATE
    SANDBOX_DATA_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

target_link_libraries(OpenGLProjeto PRIVATE rotor_engine DoomCore)

# Aplicativo independente para experimentos futuros, reutilizando o Rotor Engine.
add_executable(Dynamis
    src/applications/dynamis/main.cpp
    src/applications/dynamis/dynamis_application.cpp
)

target_link_libraries(Dynamis PRIVATE rotor_engine)

# O Launcher encontra o Dynamis pelo manifesto, sem registro manual no código.
file(GENERATE
    OUTPUT "$<TARGET_FILE_DIR:Dynamis>/dynamis.rotor-app.json"
    CONTENT "{\n  \"manifest_version\": 1,\n  \"id\": \"rotor.dynamis\",\n  \"name\": \"Dynamis\",\n  \"description\": \"Ambiente simples de experimentacao com o Rotor Engine.\",\n  \"executable\": \"$<TARGET_FILE_NAME:Dynamis>\",\n  \"rotor_api_version\": 1\n}\n"
)

# Manifesto gerado junto ao executável: o Launcher descobre o Sandbox sem
# precisar conhecer seu nome ou registrar a aplicação manualmente no código.
file(GENERATE
    OUTPUT "$<TARGET_FILE_DIR:OpenGLProjeto>/sandbox.rotor-app.json"
    CONTENT "{\n  \"manifest_version\": 1,\n  \"id\": \"rotor.sandbox\",\n  \"name\": \"Rotor Sandbox\",\n  \"description\": \"Ambiente de testes e exploracao do motor grafico Rotor.\",\n  \"executable\": \"$<TARGET_FILE_NAME:OpenGLProjeto>\",\n  \"rotor_api_version\": 1\n}\n"
)

# Inicializador independente: descobre aplicações compatíveis por manifesto.
if(WIN32)
    add_executable(RotorLauncher WIN32
        src/applications/launcher/main.cpp
        src/applications/launcher/launcher_application.cpp
        src/applications/launcher/application_discovery.cpp
        src/applications/launcher/process_launcher.cpp
    )
else()
    add_executable(RotorLauncher
        src/applications/launcher/main.cpp
        src/applications/launcher/launcher_application.cpp
        src/applications/launcher/application_discovery.cpp
        src/applications/launcher/process_launcher.cpp
    )
endif()

target_link_libraries(RotorLauncher PRIVATE rotor_engine)
add_dependencies(RotorLauncher OpenGLProjeto Dynamis)
