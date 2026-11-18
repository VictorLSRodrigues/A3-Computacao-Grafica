#pragma once

#include <string>

#include "engine/graphics/camera.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/core/settings.h"
#include "applications/sandbox/ui/HUD.h"
#include "engine/ui/UI.h"
#include "applications/sandbox/ui/light_panel.h"
#include "applications/sandbox/ui/menu.h"
#include "engine/editor/editor.h"
#include "engine/platform/window.h"
#include <glm/glm.hpp>

// Dona de tudo o que vive durante a execução: janela, cena, luz e interface.
// O main só cria a aplicação, chama init() e depois run().
class Application
{
public:
    Application() = default;

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    // Cria a janela, carrega os recursos e monta a cena. Retorna false se algo falhar.
    bool init();

    // Laço principal: roda até a janela ser fechada.
    void run();

private:
    void update(float deltaTime);
    void renderUI();

    // A janela é declarada primeiro para ser destruída por último,
    // depois dos recursos OpenGL que dependem do contexto dela.
    Window window;

    Settings settings;
    std::string selectedModelName = "Todos";

    Renderer renderer;
    Scene scene;
    Light light;

    // O tamanho real do viewport é atualizado a cada quadro em update().
    Camera camera{1, 1, glm::vec3(0.0f, 0.0f, 2.0f)};

    UI ui;

    // Usa a janela, a câmera, a cena, a luz, os ajustes e a UI acima;
    // por isso é declarado depois deles.
    Editor editor{window, camera, scene, light, settings, ui};

    Menu menu;
    LightPanel lightPanel;
    HUD hud;
};
