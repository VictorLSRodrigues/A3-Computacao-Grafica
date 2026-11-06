#include "HUD.h"

#include <cstdio>

void HUD::update(const Window& window, float deltaTime)
{
    if (window.keyPressed(GLFW_KEY_F3))
        visible = !visible;

    // Atualiza o FPS a cada meio segundo para reduzir oscilações na leitura.
    timeAccumulator += deltaTime;
    ++frameCount;

    if (timeAccumulator >= 0.5f)
    {
        fps = frameCount / timeAccumulator;
        timeAccumulator = 0.0f;
        frameCount = 0;
    }
}

void HUD::draw(UI& ui, const glm::vec3& cameraPosition)
{
    if (!visible)
        return;

    char line[64];
    const float scale = 0.4f;
    const float x = 16.0f;
    float y = 32.0f; // Linha de base do primeiro texto.

    std::snprintf(line, sizeof(line), "FPS: %.0f", fps);
    ui.drawText(line, x + 1.0f, y + 1.0f, scale, 0.0f, 0.0f, 0.0f, 0.8f); // Sombra do texto.
    ui.drawText(line, x, y, scale, 1.0f, 1.0f, 1.0f);

    y += 26.0f;
    std::snprintf(line, sizeof(line), "Pos: %.2f  %.2f  %.2f",
                  cameraPosition.x, cameraPosition.y, cameraPosition.z);
    ui.drawText(line, x + 1.0f, y + 1.0f, scale, 0.0f, 0.0f, 0.0f, 0.8f);
    ui.drawText(line, x, y, scale, 1.0f, 1.0f, 1.0f);
}