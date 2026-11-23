#pragma once

#include <glm/glm.hpp>
#include <cstddef>
#include <vector>

#include "engine/scene/transform.h"

// Simulacao do Rotor: integra movimento sob aceleracao constante.
// A simulacao anda em passos de tamanho fixo, entao o resultado nao depende do FPS.
// Ainda nao possui colisao, massa, atrito ou restricoes.
class PhysicsWorld
{
public:
    void setGravity(const glm::vec3& gravity);
    const glm::vec3& gravity() const;

    // Registra uma Transform para receber a integracao fisica.
    // A posicao e a velocidade informadas agora viram o ponto de partida (reset()).
    void addBody(Transform& transform, const glm::vec3& initialVelocity = glm::vec3(0.0f));

    // Recebe o tempo do quadro (em segundos) e simula em passos fixos.
    void update(float frameTime);

    // Volta todos os corpos a posicao e a velocidade de partida.
    void reset();

    // Passa a usar a posicao atual de cada corpo como posicao de partida
    // (util depois de mover objetos no editor).
    void recordStartPositions();

    void clear();
    std::size_t bodyCount() const;

    // Total de passos fixos ja simulados (util para diagnostico e testes).
    unsigned long long stepCount() const;

private:
    struct Body
    {
        Transform* transform = nullptr;
        glm::vec3 velocity{0.0f};
        glm::vec3 startPosition{0.0f};
        glm::vec3 startVelocity{0.0f};
    };

    // Avanca todos os corpos exatamente um passo fixo.
    void step(float dt);

    static constexpr double kFixedStep = 1.0 / 120.0; // segundos por passo
    static constexpr int kMaxStepsPerFrame = 8;       // evita a "espiral da morte"
    static constexpr double kTolerance = 1e-6;        // absorve erro de arredondamento

    glm::vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::vector<Body> bodies_;

    double accumulator_ = 0.0; // tempo ainda nao simulado
    unsigned long long stepCount_ = 0;
};