#pragma once

#include <memory>

#include "camera.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "scene/light.h"
#include "scene/scene.h"
#include "settings.h"
#include "ui/HUD.h"
#include "ui/UI.h"
#include "ui/menu.h"
#include "window.h"

// Dona de tudo o que vive durante a execução: janela, recursos, cena e interface.
class Application
{
public:
    Application() = default;

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    bool init(); // Cria a janela, carrega recursos e monta a cena.
    void run();  // Laço principal: roda até a janela ser fechada.

private:
    bool loadShaders();
    bool loadScene();

    void update(float deltaTime);
    void render();
    void renderUI();

    // A janela vem primeiro para ser destruída por último,
    // depois dos recursos OpenGL que dependem do contexto dela.
    Window window;

    Settings settings;

    Shader sceneShader;
    Shader lightShader;
    Texture defaultTexture;

    Scene scene;
    Scene::Object *pyramid = nullptr;
    std::unique_ptr<Light> light; // Criados em init(): o construtor deles usa OpenGL.

    Camera camera{1, 1, glm::vec3(0.0f, 0.0f, 2.0f)};

    std::unique_ptr<UI> ui;
    Menu menu;
    HUD hud;

    float pyramidRotation = 0.0f;
};