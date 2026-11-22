#pragma once

#include <glm/glm.hpp>
#include <cstddef>
#include <vector>

#include "engine/scene/transform.h"

// Simulacao inicial do Rotor: integra movimento sob aceleracao constante.
// Ainda nao possui colisao, massa, atrito ou restricoes.
class PhysicsWorld
{
public:
    void setGravity(const glm::vec3& gravity);
    const glm::vec3& gravity() const;

    // Registra uma Transform para receber a integracao fisica.
    void addBody(Transform& transform, const glm::vec3& initialVelocity = glm::vec3(0.0f));

    // Avanca todos os corpos pelo tempo do quadro (em segundos).
    void update(float deltaTime);

    void clear();
    std::size_t bodyCount() const;

private:
    struct Body
    {
        Transform* transform = nullptr;
        glm::vec3 velocity{0.0f};
    };

    glm::vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::vector<Body> bodies_;
};
