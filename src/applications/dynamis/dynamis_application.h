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
    Camera camera{1280, 800, glm::vec3(8.0f, 8.0f, 10.0f)};
    UI ui;
    Editor editor{window, camera, scene, light, ui};
};
