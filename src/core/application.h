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

    // Estado do arraste do gizmo de movimentação.
    bool draggingGizmo = false;
    int draggedAxis = -1;

    glm::vec3 dragStartPosition{0.0f};
    glm::vec3 draggedWorldAxis{0.0f};

    glm::vec2 dragStartMouse{0.0f};
    glm::vec2 dragScreenDirection{0.0f};

    // Estado do arraste do gizmo de rotação.
    bool draggingRotationGizmo = false;
    int rotatingAxis = -1;

    // Guarda a rotação original do objeto.
    glm::vec3 rotationStart{0.0f};

    // Guarda o vetor inicial do mouse no plano de rotação.
    glm::vec3 rotationStartVector{0.0f};

    // Guarda o eixo 3D escolhido.
    glm::vec3 rotatingWorldAxis{0.0f};

    float rotationAccumulatedAngle = 0.0f;

    float dragWorldPerPixel = 0.0f;

    void drawTransformToolbar();
    void drawMoveGizmo();
    void drawRotateGizmo();
};