#pragma once

#include <string>

#include "camera.h"
#include "renderer.h"
#include "scene/light.h"
#include "scene/scene.h"
#include "settings.h"
#include "ui/HUD.h"
#include "ui/UI.h"
#include "ui/menu.h"
#include "window.h"

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
    bool editingMode = false;
    std::string selectedModelName = "Todos";

    Renderer renderer;
    Scene scene;
    Light light;

    // O tamanho real do viewport é atualizado a cada quadro em update().
    Camera camera{1, 1, glm::vec3(0.0f, 0.0f, 2.0f)};

    UI ui;
    Menu menu;
    HUD hud;

    enum class TransformTool
    {
        Move,
        Rotate,
        Scale
    };

    TransformTool activeTransformTool = TransformTool::Move;

    void drawTransformToolbar();
};