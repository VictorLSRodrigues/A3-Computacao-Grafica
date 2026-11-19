#pragma once

#include <glm/glm.hpp>

// Propriedades de superfície usadas pelo shader de iluminação atual.
// metallic/roughness são compatíveis com os fatores do padrão glTF 2.0.
struct Material
{
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor = 0.0f;
    float roughnessFactor = 1.0f;
    float shininess = 32.0f;
};
