#pragma once

enum class LightType
{
    Point = 0,
    Directional = 1,
    Spot = 2
};

// Valores que o jogador pode ajustar nas opções.
struct Settings
{
    float fov = 45.0f;               // campo de visão, em graus
    float walkSpeed = 1.5f;          // velocidade da câmera
    float sprintSpeed = 5.0f;        // velocidade da câmera com Shift
    float mouseSensitivity = 100.0f; // sensibilidade do mouse
    LightType lightType = LightType::Point;
};