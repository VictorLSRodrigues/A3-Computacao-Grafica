#pragma once

// Configurações de câmera e renderização compartilhadas pelo app e pela interface.
// Os dados da iluminação ficam na Light.
struct Settings
{
    float fov = 45.0f;               // campo de visão, em graus
    float walkSpeed = 1.5f;          // velocidade da câmera
    float sprintSpeed = 5.0f;        // velocidade da câmera com Shift
    float mouseSensitivity = 100.0f; // sensibilidade do mouse
    bool showSkybox = true;          // mostra o céu ao redor da cena
};