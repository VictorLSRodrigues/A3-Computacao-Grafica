#pragma once

#include <glm/glm.hpp>

#include "engine/graphics/camera/camera.h"
#include "engine/graphics/mesh/mesh.h"
#include "engine/graphics/resources/shader.h"

// Tipo da fonte de luz; o valor inteiro é enviado ao shader.
enum class LightType
{
    Point = 0,
    Directional = 1,
    Spot = 2
};

// Fonte de luz da cena: guarda tipo, cor, posição e alvo e desenha um cubo que marca onde ela está.
class Light
{
public:
    LightType type = kDefaultType;
    glm::vec3 color = kDefaultColor; // RGB, de 0 a 1
    glm::vec3 position = kDefaultPosition;
    glm::vec3 target = kDefaultTarget; // ponto para onde a luz aponta

    bool create();

    // Volta tipo, cor, posição e alvo aos valores padrão.
    void resetProperties();

    // Direção em que a luz viaja (da posição até o alvo), com comprimento 1.
    glm::vec3 direction() const;

    // Envia ao shader da cena os dados da luz e da câmera usados na iluminação.
    void apply(Shader &shader, const Camera &camera) const;

    // Desenha o cubo que indica a posição da luz. O shader precisa já ter a matriz da câmera.
    void drawMarker(Shader &markerShader) const;

private:
    static constexpr LightType kDefaultType = LightType::Point;
    static inline const glm::vec3 kDefaultColor{1.0f, 1.0f, 1.0f};
    static inline const glm::vec3 kDefaultPosition{0.5f, 0.5f, 0.5f};
    static inline const glm::vec3 kDefaultTarget{0.0f, 0.0f, 0.0f};

    Mesh marker;
};
