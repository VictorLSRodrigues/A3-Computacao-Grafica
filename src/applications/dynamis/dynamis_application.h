#pragma once

#include "engine/application/application.h"
#include "engine/editor/editor.h"
#include "engine/graphics/camera/camera.h"
#include "engine/renderer/renderer.h"
#include "engine/physics/physics_world.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/ui/UI.h"

class DynamisApplication : public rotor::Application
{
public:
    DynamisApplication();

protected:
    bool onInitialize() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;

private:
    Renderer renderer;
    Scene scene;
    PhysicsWorld physics;
    Light light;
    Camera camera{1280, 800, glm::vec3(0.0f, 20.0f, 42.0f)};
    UI ui;
    Editor editor{window, camera, scene, light, ui};
    Scene::Object* followedObject = nullptr;
    Transform* forceControlledTransform = nullptr;
};
