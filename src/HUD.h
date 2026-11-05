#pragma once

#include <glm/glm.hpp>

#include "UI.h"
#include "window.h"

// Informações de depuração no canto da tela (FPS e posição da câmera). F3 liga e desliga.
class HUD
{
public:
    void update(const Window& window, float deltaTime);
    void draw(UI& ui, const glm::vec3& cameraPosition);

private:
    bool visible = true;
    float fps = 0.0f;
    float timeAccumulator = 0.0f;
    int frameCount = 0;
};