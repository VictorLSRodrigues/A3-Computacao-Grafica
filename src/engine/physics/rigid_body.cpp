#include "engine/physics/physics_types.h"

#include <algorithm>
#include <cmath>
#include <glm/gtx/quaternion.hpp>

namespace
{
bool isFiniteVector(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}
}

void rotor::physics::RigidBody::synchronizeColliderAndInertia()
{
    if (!transform)
    {
        collider.sphereRadius = 0.0f;
        collider.boxHalfExtents = glm::vec3(0.0f);
        inverseInertiaLocal = glm::mat3(0.0f);
        return;
    }

    const glm::vec3 scale = glm::abs(transform->scale());
    if (!isFiniteVector(scale))
    {
        collider.sphereRadius = 0.0f;
        collider.boxHalfExtents = glm::vec3(0.0f);
        inverseInertiaLocal = glm::mat3(0.0f);
        return;
    }

    if (collider.shape == ColliderShape::Sphere)
    {
        const float scaleFactor = std::max(
            scale.x, std::max(scale.y, scale.z));
        const float scaledRadius = collider.baseSphereRadius * scaleFactor;
        if (!std::isfinite(scaledRadius))
        {
            collider.sphereRadius = 0.0f;
            inverseInertiaLocal = glm::mat3(0.0f);
            return;
        }

        collider.sphereRadius = scaledRadius;
        collider.boxHalfExtents = glm::vec3(0.0f);
        if (inverseMass > 0.0f && collider.sphereRadius > 1e-6f)
        {
            const float inverseInertia =
                2.5f * inverseMass /
                (collider.sphereRadius * collider.sphereRadius);
            inverseInertiaLocal = glm::mat3(inverseInertia);
        }
        else
        {
            inverseInertiaLocal = glm::mat3(0.0f);
        }
        return;
    }

    collider.sphereRadius = 0.0f;
    collider.boxHalfExtents = collider.baseBoxHalfExtents * scale;
    if (inverseMass > 0.0f &&
        collider.boxHalfExtents.x > 1e-6f &&
        collider.boxHalfExtents.y > 1e-6f &&
        collider.boxHalfExtents.z > 1e-6f)
    {
        const glm::vec3 h = collider.boxHalfExtents;
        // Inércia de uma caixa maciça: Ixx = m/3 * (hy² + hz²), etc.
        const glm::vec3 inverseInertia(
            3.0f * inverseMass / (h.y * h.y + h.z * h.z),
            3.0f * inverseMass / (h.x * h.x + h.z * h.z),
            3.0f * inverseMass / (h.x * h.x + h.y * h.y));
        inverseInertiaLocal = glm::mat3(0.0f);
        inverseInertiaLocal[0][0] = inverseInertia.x;
        inverseInertiaLocal[1][1] = inverseInertia.y;
        inverseInertiaLocal[2][2] = inverseInertia.z;
    }
    else
    {
        inverseInertiaLocal = glm::mat3(0.0f);
    }
}

glm::mat3 rotor::physics::RigidBody::inverseInertiaTensorWorld() const
{
    if (!transform || inverseMass <= 0.0f)
        return glm::mat3(0.0f);

    const glm::mat3 rotation = glm::mat3_cast(transform->orientation());
    return rotation * inverseInertiaLocal * glm::transpose(rotation);
}
