#include "engine/physics/physics_types.h"

#include <algorithm>
#include <cmath>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>

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
        collider.boxCenter = glm::vec3(0.0f);
        inverseInertiaLocal = glm::mat3(0.0f);
        hasSynchronizedScale = false;
        return;
    }

    const glm::vec3 signedScale = transform->scale();
    const glm::vec3 scale = glm::abs(signedScale);
    if (!isFiniteVector(scale))
    {
        collider.sphereRadius = 0.0f;
        collider.boxHalfExtents = glm::vec3(0.0f);
        collider.boxCenter = glm::vec3(0.0f);
        inverseInertiaLocal = glm::mat3(0.0f);
        hasSynchronizedScale = false;
        return;
    }

    const bool unchangedScale =
        scale.x == lastSynchronizedScale.x &&
        scale.y == lastSynchronizedScale.y &&
        scale.z == lastSynchronizedScale.z;
    const bool unchangedBox =
        collider.baseBoxHalfExtents.x == lastSynchronizedBoxHalfExtents.x &&
        collider.baseBoxHalfExtents.y == lastSynchronizedBoxHalfExtents.y &&
        collider.baseBoxHalfExtents.z == lastSynchronizedBoxHalfExtents.z;
    const bool unchangedSignedScale =
        signedScale.x == lastSynchronizedSignedScale.x &&
        signedScale.y == lastSynchronizedSignedScale.y &&
        signedScale.z == lastSynchronizedSignedScale.z;
    const bool unchanged = hasSynchronizedScale && unchangedScale &&
        unchangedSignedScale && unchangedBox &&
        collider.baseSphereRadius == lastSynchronizedSphereRadius &&
        inverseMass == lastSynchronizedInverseMass &&
        collider.shape == lastSynchronizedShape &&
        collider.triangleMesh.get() == lastSynchronizedMesh;
    if (unchanged)
        return;

    lastSynchronizedScale = scale;
    lastSynchronizedSignedScale = signedScale;
    lastSynchronizedBoxHalfExtents = collider.baseBoxHalfExtents;
    lastSynchronizedSphereRadius = collider.baseSphereRadius;
    lastSynchronizedInverseMass = inverseMass;
    lastSynchronizedShape = collider.shape;
    lastSynchronizedMesh = collider.triangleMesh.get();
    hasSynchronizedScale = true;

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
        collider.boxCenter = glm::vec3(0.0f);
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

    if (collider.shape == ColliderShape::TriangleMesh)
    {
        collider.boxCenter = glm::vec3(0.0f);
        if (!collider.triangleMesh || !collider.triangleMesh->valid() ||
            inverseMass <= 0.0f)
        {
            inverseInertiaLocal = glm::mat3(0.0f);
            return;
        }

        // A malha guarda a covariância da distribuição superficial unitária.
        // Aplicamos a escala nos dois lados para obter a inércia no espaço local
        // atual, sem reduzir todo o objeto a uma caixa maciça homogênea.
        glm::mat3 scaleMatrix(1.0f);
        scaleMatrix[0][0] = signedScale.x;
        scaleMatrix[1][1] = signedScale.y;
        scaleMatrix[2][2] = signedScale.z;
        collider.boxCenter = collider.triangleMesh->localBoundsCenter * signedScale;
        const glm::mat3 covariance =
            scaleMatrix * collider.triangleMesh->unitMassCovariance * scaleMatrix;
        const float covarianceTrace =
            covariance[0][0] + covariance[1][1] + covariance[2][2];
        glm::mat3 unitInertia = glm::mat3(covarianceTrace) - covariance;

        // Malhas abertas ou quase planas podem produzir um tensor mal condicionado.
        // Uma regularização pequena evita inversões singulares sem dominar a forma.
        const float maximumExtent = std::max(
            collider.boxHalfExtents.x,
            std::max(collider.boxHalfExtents.y, collider.boxHalfExtents.z));
        const float regularization = std::max(
            1e-9f, maximumExtent * maximumExtent * 1e-6f);
        unitInertia[0][0] += regularization;
        unitInertia[1][1] += regularization;
        unitInertia[2][2] += regularization;

        const float determinant = glm::determinant(unitInertia);
        if (std::isfinite(determinant) && determinant > 1e-15f)
            inverseInertiaLocal = glm::inverse(unitInertia) * inverseMass;
        else
            inverseInertiaLocal = glm::mat3(0.0f);
        return;
    }

    collider.boxCenter = glm::vec3(0.0f);
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
