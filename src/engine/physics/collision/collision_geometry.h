#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/physics/physics_types.h"

namespace rotor::physics::collision
{
struct OrientedBox
{
    glm::vec3 center{0.0f};
    glm::vec3 axes[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)};
    glm::vec3 halfExtents{0.0f};
};

// Amostra geométrica da superfície da bacia mais próxima do corpo consultado.
struct BowlSurfaceSample
{
    glm::vec3 surfacePoint{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float signedDistance = 0.0f;
};

OrientedBox makeOrientedBox(
    const glm::vec3& center,
    const glm::quat& orientation,
    const glm::vec3& halfExtents);

float projectedRadius(const OrientedBox& box, const glm::vec3& axis);
glm::vec3 supportPoint(const OrientedBox& box, const glm::vec3& direction);
// Consultas de suporte usam a forma efetiva do colisor e sua Transform.
glm::vec3 supportPoint(const RigidBody& body, const glm::vec3& direction);
float supportRadius(const RigidBody& body, const glm::vec3& direction);

// Gera um manifold com até quatro pontos para colisões caixa-caixa.
bool intersectOrientedBoxes(
    const OrientedBox& a,
    const OrientedBox& b,
    ContactManifold& manifold);

// Compatibilidade para consultas simples que precisam de somente um ponto.
bool intersectOrientedBoxes(
    const OrientedBox& a,
    const OrientedBox& b,
    ContactPoint& contact);

// Gera os vértices de contato de uma caixa com um plano infinito.
bool boxPlaneContacts(
    const OrientedBox& box,
    const glm::vec3& planeNormal,
    float planeOffset,
    ContactManifold& manifold);

bool sphereOrientedBoxContact(
    const glm::vec3& sphereCenter,
    float sphereRadius,
    const OrientedBox& box,
    ContactPoint& contact);

bool sphereSphereContact(
    const glm::vec3& centerA,
    float radiusA,
    const glm::vec3& centerB,
    float radiusB,
    ContactPoint& contact);

BowlSurfaceSample findClosestBowlSurface(
    const glm::vec3& position,
    float radius,
    float depth,
    float lipHeight);
} // namespace rotor::physics::collision
