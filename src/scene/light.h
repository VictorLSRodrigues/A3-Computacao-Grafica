#pragma once

#include <glm/glm.hpp>

#include "camera.h"
#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "settings.h"

// Fonte de luz da cena: guarda cor e posição e desenha um cubo que marca onde ela está.
class Light
{
public:
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec3 position{0.5f, 0.5f, 0.5f};
    glm::vec3 target{0.0f, 0.0f, 0.0f}; // ponto para onde a luz aponta

    bool create();

    // Direção em que a luz viaja (da posição até o alvo), com comprimento 1.
    glm::vec3 direction() const;

    // Envia ao shader da cena os dados da luz e da câmera usados na iluminação.
    void apply(Shader &shader, LightType type, const Camera &camera) const;

    // Desenha o cubo que indica a posição da luz (não faz sentido na luz direcional).
    void drawMarker(Shader &markerShader, Camera &camera, float fov);

private:
    Mesh marker;
};
