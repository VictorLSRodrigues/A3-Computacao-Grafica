#pragma once

#include <glm/glm.hpp>

// Tipo da fonte de luz; o valor inteiro é enviado ao shader.
enum class LightType
{
    Point = 0,
    Directional = 1,
    Spot = 2
};

// Fonte de luz da cena: guarda suas propriedades e cálculos próprios.
class Light
{
public:
    LightType type = kDefaultType;
    glm::vec3 color = kDefaultColor; // RGB, de 0 a 1
    float intensity = 1.0f;
    glm::vec3 position = kDefaultPosition;
    glm::vec3 target = kDefaultTarget; // ponto para onde a luz aponta

    // Volta tipo, cor, posição, alvo e intensidade aos valores padrão.
    void resetProperties();

    // Direção em que a luz viaja (da posição até o alvo), com comprimento 1.
    glm::vec3 direction() const;

private:
    static constexpr LightType kDefaultType = LightType::Point;
    static inline const glm::vec3 kDefaultColor{1.0f, 1.0f, 1.0f};
    static inline const glm::vec3 kDefaultPosition{0.5f, 0.5f, 0.5f};
    static inline const glm::vec3 kDefaultTarget{0.0f, 0.0f, 0.0f};
    static constexpr float kDefaultIntensity = 1.0f;
};
