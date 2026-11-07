#pragma once
#include <glm/glm.hpp>

enum class LightType
{
    Point = 0,
    Directional = 1,
    Spot = 2
};

// Opções de câmera e iluminação ajustáveis pelo menu.
struct Settings
{
    float fov = 45.0f;               // campo de visão, em graus
    float walkSpeed = 1.5f;          // velocidade da câmera
    float sprintSpeed = 5.0f;        // velocidade da câmera com Shift
    float mouseSensitivity = 100.0f; // sensibilidade do mouse
    LightType lightType = LightType::Point;
    glm::vec3 lightPosition{0.5f, 0.5f, 0.5f}; // onde a luz está
    glm::vec3 lightTarget{0.0f, 0.0f, 0.0f};   // ponto para onde a luz aponta
};