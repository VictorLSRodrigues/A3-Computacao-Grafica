#pragma once

#include "engine/application/application.h"
#include "engine/editor/editor.h"
#include "engine/graphics/camera/camera.h"
#include "engine/renderer/renderer.h"
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
    Light light;
    Camera camera{1280, 800, glm::vec3(0.0f, 0.0f, 5.0f)};
    UI ui;
    Editor editor{window, camera, scene, light, ui};
};
