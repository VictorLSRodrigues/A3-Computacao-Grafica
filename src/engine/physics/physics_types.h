#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <array>
#include <cstddef>

#include "engine/scene/transform.h"

namespace rotor::physics
{
// Propriedades de superfície compartilhadas entre corpos e colisores estáticos.
struct PhysicsMaterial
{
    float restitution = 0.0f;
    float staticFriction = 0.6f;
    float dynamicFriction = 0.4f;
};

enum class ColliderShape
{
    Sphere,
    Box
};

// Geometria de colisão associada a um corpo. Dimensões base são locais;
// dimensões efetivas são atualizadas quando a escala visual muda.
struct Collider
{
    ColliderShape shape = ColliderShape::Sphere;
    float baseSphereRadius = 0.0f;
    float sphereRadius = 0.0f;
    glm::vec3 baseBoxHalfExtents{0.0f};
    glm::vec3 boxHalfExtents{0.0f};
};

// Estado dinâmico de um corpo rígido. A referência a Transform é, por enquanto,
// o vínculo usado pelo PhysicsWorld para sincronizar com aplicações existentes.
// A física não depende de Mesh, Model, Renderer ou Scene::Object.
struct RigidBody
{
    Transform* transform = nullptr;
    Collider collider;
    PhysicsMaterial material;

    glm::vec3 velocity{0.0f};
    glm::vec3 angularVelocity{0.0f};
    glm::vec3 forceAccumulator{0.0f};
    glm::vec3 torqueAccumulator{0.0f};
    glm::vec3 continuousForce{0.0f};
    glm::vec3 continuousTorque{0.0f};

    glm::vec3 startPosition{0.0f};
    glm::vec3 startVelocity{0.0f};
    glm::vec3 startAngularVelocity{0.0f};
    glm::quat startOrientation{1.0f, 0.0f, 0.0f, 0.0f};

    float inverseMass = 1.0f;
    // Tensor de inércia inverso diagonal em espaço local.
    glm::mat3 inverseInertiaLocal{0.0f};

    // Atualiza dimensões efetivas do colisor e o tensor de inércia após mudanças de escala.
    void synchronizeColliderAndInertia();
    glm::mat3 inverseInertiaTensorWorld() const;
};

// Colisor estático de caixa ligado a um Transform de aplicação.
struct StaticBoxCollider
{
    Transform* transform = nullptr;
    glm::vec3 localCenter{0.0f};
    glm::vec3 localHalfExtents{0.0f};
    PhysicsMaterial material;
};

// Um ponto de contato produzido por uma consulta geométrica. A normal aponta
// de A para B em pares de corpos; em contato corpo-superfície, aponta da
// superfície estática para o corpo dinâmico.
struct ContactPoint
{
    // Ponto médio para depuração/visualização e compatibilidade.
    glm::vec3 point{0.0f};
    // Pontos independentes nas superfícies de A e B. O ponto A pode pertencer
    // a um colisor estático quando o corpo A do solver é nulo.
    glm::vec3 pointA{0.0f};
    glm::vec3 pointB{0.0f};
    bool hasSeparatePoints = false;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float penetration = 0.0f;

    // Impulsos acumulados durante as iterações do solver deste passo físico.
    float accumulatedNormalImpulse = 0.0f;
    glm::vec3 accumulatedTangentImpulse{0.0f};
    float restitutionBias = 0.0f;
};

// Conjunto pequeno de pontos que descreve uma única região de contato.
// Quatro pontos são suficientes para contatos face-a-face entre caixas.
struct ContactManifold
{
    static constexpr std::size_t MaxPoints = 4;
    std::array<ContactPoint, MaxPoints> points{};
    std::size_t pointCount = 0;

    bool empty() const { return pointCount == 0; }
    void clear() { pointCount = 0; }
};
} // namespace rotor::physics
