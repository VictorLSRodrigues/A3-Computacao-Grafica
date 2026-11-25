#pragma once

#include <string>
#include <deque>

#include "engine/application/application.h"
#include "engine/graphics/camera/camera.h"
#include "engine/renderer/renderer.h"
#include "engine/physics/physics_world.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "applications/sandbox/ui/HUD.h"
#include "engine/ui/UI.h"
#include "applications/sandbox/ui/light_panel.h"
#include "applications/sandbox/ui/menu.h"
#include "applications/sandbox/doom/doom_game.h"
#include "engine/editor/editor.h"
#include <glm/glm.hpp>

// Aplicacao de demonstracao e ferramentas do Rotor.
// A janela e o ciclo principal sao administrados por rotor::Application.
class SandboxApplication : public rotor::Application
{
public:
    SandboxApplication();

protected:
    bool onInitialize() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    void onShutdown() override;

    // O menu do Sandbox ja interpreta Escape: abre/fecha o menu em vez de
    // encerrar a aplicacao diretamente.
    void onEscapePressed() override {}

private:
    void renderUI();
    void startDoom();
    bool registerScenePhysics(bool stabilizeOverlaps = true);
    void syncSceneFromPhysics();
    void resetPhysicsFromScene(bool stabilizeOverlaps = true);

    struct PhysicsBinding
    {
        Scene::Object *object = nullptr;
        Transform bodyTransform;
        glm::vec3 localCenterOfMass{0.0f};
    };

    std::string selectedModelName = "Todos";

    Renderer renderer;
    Scene scene;
    rotor::physics::PhysicsWorld physics;
    // rotor::physics::PhysicsWorld guarda ponteiros para estas Transform. deque mantém
    // os endereços estáveis enquanto os vínculos são registrados.
    std::deque<PhysicsBinding> physicsBindings;
    Light light;
    Camera camera{1, 1, glm::vec3(0.0f, 0.0f, 2.0f)};
    UI ui;

    // O Editor usa recursos declarados anteriormente e a janela da classe base.
    Editor editor{window, camera, scene, light, ui};

    Menu menu;
    LightPanel lightPanel;
    HUD hud;
    DoomGame doomGame;
    bool doomActive = false;
    bool sceneReadyForPersistence = false;
};
