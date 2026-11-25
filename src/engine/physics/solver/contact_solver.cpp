#include "engine/physics/solver/contact_solver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/gtx/norm.hpp>

namespace rotor::physics::solver
{
namespace
{
constexpr float kRestitutionVelocityThreshold = 0.5f;
constexpr float kPenetrationSlop = 0.000025f;

float clampFriction(float coefficient)
{
    return std::clamp(coefficient, 0.0f, 1.0f);
}

glm::vec3 pointOnA(const ContactPoint& contact)
{
    return contact.hasSeparatePoints ? contact.pointA : contact.point;
}

glm::vec3 pointOnB(const ContactPoint& contact)
{
    return contact.hasSeparatePoints ? contact.pointB : contact.point;
}

glm::vec3 velocityAtPoint(
    const RigidBody* body,
    const glm::vec3& point)
{
    if (!body || !body->transform)
        return glm::vec3(0.0f);

    const glm::vec3 arm = point - body->transform->position();
    return body->velocity + glm::cross(body->angularVelocity, arm);
}

glm::mat3 inverseInertia(const RigidBody* body)
{
    return body ? body->inverseInertiaTensorWorld() : glm::mat3(0.0f);
}
float inverseMass(const RigidBody* body)
{
    return body ? std::max(0.0f, body->inverseMass) : 0.0f;
}

float effectiveMassDenominator(
    const RigidBody* bodyA,
    const RigidBody* bodyB,
    const glm::vec3& armA,
    const glm::vec3& armB,
    const glm::vec3& direction,
    const glm::mat3& inverseInertiaA,
    const glm::mat3& inverseInertiaB)
{
    const glm::vec3 angularA = glm::cross(armA, direction);
    const glm::vec3 angularB = glm::cross(armB, direction);
    return inverseMass(bodyA) + inverseMass(bodyB) +
        glm::dot(angularA, inverseInertiaA * angularA) +
        glm::dot(angularB, inverseInertiaB * angularB);
}

void applyPairImpulse(
    RigidBody* bodyA,
    RigidBody* bodyB,
    const glm::vec3& armA,
    const glm::vec3& armB,
    const glm::mat3& inverseInertiaA,
    const glm::mat3& inverseInertiaB,
    const glm::vec3& impulseOnB)
{
    if (bodyA && bodyA->transform && bodyA->inverseMass > 0.0f)
    {
        bodyA->velocity -= impulseOnB * bodyA->inverseMass;
        bodyA->angularVelocity -= inverseInertiaA * glm::cross(armA, impulseOnB);
    }

    if (bodyB && bodyB->transform && bodyB->inverseMass > 0.0f)
    {
        bodyB->velocity += impulseOnB * bodyB->inverseMass;
        bodyB->angularVelocity += inverseInertiaB * glm::cross(armB, impulseOnB);
    }
}

void resetAccumulatedImpulses(ContactManifold& manifold)
{
    for (std::size_t i = 0; i < manifold.pointCount; ++i)
    {
        manifold.points[i].accumulatedNormalImpulse = 0.0f;
        manifold.points[i].accumulatedTangentImpulse = glm::vec3(0.0f);
        manifold.points[i].restitutionBias = 0.0f;
    }
}

bool solveLinearSystem(
    double matrix[ContactManifold::MaxPoints][ContactManifold::MaxPoints + 1],
    std::size_t dimension,
    double solution[ContactManifold::MaxPoints])
{
    for (std::size_t column = 0; column < dimension; ++column)
    {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < dimension; ++row)
        {
            if (std::fabs(matrix[row][column]) >
                std::fabs(matrix[pivot][column]))
            {
                pivot = row;
            }
        }

        if (std::fabs(matrix[pivot][column]) <= 1e-10)
            return false;

        if (pivot != column)
        {
            for (std::size_t entry = column; entry <= dimension; ++entry)
                std::swap(matrix[pivot][entry], matrix[column][entry]);
        }

        const double divisor = matrix[column][column];
        for (std::size_t entry = column; entry <= dimension; ++entry)
            matrix[column][entry] /= divisor;

        for (std::size_t row = 0; row < dimension; ++row)
        {
            if (row == column)
                continue;
            const double factor = matrix[row][column];
            for (std::size_t entry = column; entry <= dimension; ++entry)
                matrix[row][entry] -= factor * matrix[column][entry];
        }
    }

    for (std::size_t row = 0; row < dimension; ++row)
        solution[row] = matrix[row][dimension];
    return true;
}

bool solveNormalImpulseLcp(
    const double matrix[ContactManifold::MaxPoints][ContactManifold::MaxPoints],
    const double offset[ContactManifold::MaxPoints],
    std::size_t count,
    double solution[ContactManifold::MaxPoints])
{
    if (count == 0 || count > ContactManifold::MaxPoints)
        return false;

    const unsigned int subsetCount = 1u << static_cast<unsigned int>(count);
    double bestError = std::numeric_limits<double>::max();
    bool foundSolution = false;

    for (unsigned int mask = 0; mask < subsetCount; ++mask)
    {
        std::size_t active[ContactManifold::MaxPoints]{};
        std::size_t activeCount = 0;
        for (std::size_t i = 0; i < count; ++i)
        {
            if ((mask & (1u << static_cast<unsigned int>(i))) != 0u)
                active[activeCount++] = i;
        }

        double candidate[ContactManifold::MaxPoints]{};
        if (activeCount > 0)
        {
            double system[ContactManifold::MaxPoints]
                         [ContactManifold::MaxPoints + 1]{};
            for (std::size_t row = 0; row < activeCount; ++row)
            {
                for (std::size_t column = 0; column < activeCount; ++column)
                    system[row][column] = matrix[active[row]][active[column]];
                system[row][activeCount] = -offset[active[row]];
            }

            double activeSolution[ContactManifold::MaxPoints]{};
            if (!solveLinearSystem(system, activeCount, activeSolution))
                continue;

            bool nonNegative = true;
            for (std::size_t i = 0; i < activeCount; ++i)
            {
                if (activeSolution[i] < -1e-7)
                {
                    nonNegative = false;
                    break;
                }
                candidate[active[i]] = std::max(0.0, activeSolution[i]);
            }
            if (!nonNegative)
                continue;
        }

        bool feasible = true;
        double complementarityError = 0.0;
        for (std::size_t i = 0; i < count; ++i)
        {
            double residual = offset[i];
            for (std::size_t j = 0; j < count; ++j)
                residual += matrix[i][j] * candidate[j];

            if (residual < -1e-7)
            {
                feasible = false;
                break;
            }
            complementarityError = std::max(
                complementarityError, std::fabs(candidate[i] * residual));
        }

        if (feasible && complementarityError < bestError)
        {
            bestError = complementarityError;
            for (std::size_t i = 0; i < count; ++i)
                solution[i] = candidate[i];
            foundSolution = true;
        }
    }

    return foundSolution;
}
} // namespace

PhysicsMaterial combineMaterials(
    const PhysicsMaterial& a,
    const PhysicsMaterial& b)
{
    PhysicsMaterial combined;
    combined.restitution = std::min(a.restitution, b.restitution);
    combined.staticFriction = std::sqrt(std::max(
        0.0f, a.staticFriction * b.staticFriction));
    combined.dynamicFriction = std::sqrt(std::max(
        0.0f, a.dynamicFriction * b.dynamicFriction));
    combined.staticFriction = clampFriction(combined.staticFriction);
    combined.dynamicFriction = std::min(
        clampFriction(combined.dynamicFriction), combined.staticFriction);
    return combined;
}

void prepareContactManifold(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material)
{
    resetAccumulatedImpulses(manifold);
    if (manifold.empty())
        return;

    const float restitution = std::clamp(material.restitution, 0.0f, 1.0f);

    // Cada ponto recebe seu próprio alvo de restituição, calculado a partir da
    // velocidade relativa antes de qualquer impulso do solver. Não distribuímos
    // um impulso agregado pelo centroide: isso mascara os braços de alavanca e
    // pode introduzir torque em contatos simétricos.
    for (std::size_t i = 0; i < manifold.pointCount; ++i)
    {
        ContactPoint& contact = manifold.points[i];
        const glm::vec3 relativeVelocity =
            velocityAtPoint(bodyB, pointOnB(contact)) -
            velocityAtPoint(bodyA, pointOnA(contact));
        const float normalVelocity = glm::dot(relativeVelocity, contact.normal);

        contact.restitutionBias = normalVelocity < -kRestitutionVelocityThreshold
            ? -restitution * normalVelocity
            : 0.0f;
    }
}

void prepareContactSolverCache(
    RigidBody* bodyA,
    RigidBody* bodyB,
    const ContactManifold& manifold,
    ContactSolverCache& cache)
{
    cache = ContactSolverCache{};
    cache.pointCount = std::min(
        manifold.pointCount, ContactManifold::MaxPoints);
    cache.inverseMassA = inverseMass(bodyA);
    cache.inverseMassB = inverseMass(bodyB);
    cache.inverseInertiaA = inverseInertia(bodyA);
    cache.inverseInertiaB = inverseInertia(bodyB);

    for (std::size_t i = 0; i < cache.pointCount; ++i)
    {
        const ContactPoint& contact = manifold.points[i];
        cache.armsA[i] = bodyA && bodyA->transform
            ? pointOnA(contact) - bodyA->transform->position() : glm::vec3(0.0f);
        cache.armsB[i] = bodyB && bodyB->transform
            ? pointOnB(contact) - bodyB->transform->position() : glm::vec3(0.0f);
        const float normalLength = glm::length(contact.normal);
        cache.normals[i] = normalLength > 1e-7f
            ? contact.normal / normalLength : glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // A matriz de massa efetiva depende apenas da geometria do manifold,
    // massas e orientação dos corpos. Recalculá-la em cada iteração era
    // trabalho redundante: nenhuma dessas grandezas muda no solver de impulso.
    for (std::size_t i = 0; i < cache.pointCount; ++i)
    {
        const glm::vec3 angularArmA = glm::cross(
            cache.armsA[i], cache.normals[i]);
        const glm::vec3 angularArmB = glm::cross(
            cache.armsB[i], cache.normals[i]);
        for (std::size_t j = 0; j < cache.pointCount; ++j)
        {
            const glm::vec3 angularArmAj = glm::cross(
                cache.armsA[j], cache.normals[j]);
            const glm::vec3 angularArmBj = glm::cross(
                cache.armsB[j], cache.normals[j]);
            const float coupling =
                (cache.inverseMassA + cache.inverseMassB) *
                    glm::dot(cache.normals[i], cache.normals[j]) +
                glm::dot(angularArmA,
                    cache.inverseInertiaA * angularArmAj) +
                glm::dot(angularArmB,
                    cache.inverseInertiaB * angularArmBj);
            cache.normalMatrix[i][j] = static_cast<double>(coupling);
        }
    }
}

float solveContactVelocity(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material,
    const ContactSolverCache& cache)
{
    const std::size_t count = std::min(
        manifold.pointCount, cache.pointCount);
    if (count == 0)
        return 0.0f;

    double normalOffset[ContactManifold::MaxPoints]{};
    double accumulated[ContactManifold::MaxPoints]{};
    double solved[ContactManifold::MaxPoints]{};
    float maximumImpulseChange = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        ContactPoint& contact = manifold.points[i];
        accumulated[i] = contact.accumulatedNormalImpulse;
        const glm::vec3 relativeVelocity =
            velocityAtPoint(bodyB, pointOnB(contact)) -
            velocityAtPoint(bodyA, pointOnA(contact));
        normalOffset[i] = static_cast<double>(
            glm::dot(relativeVelocity, cache.normals[i]) - contact.restitutionBias);
    }

    for (std::size_t i = 0; i < count; ++i)
    {
        for (std::size_t j = 0; j < count; ++j)
            normalOffset[i] -= cache.normalMatrix[i][j] * accumulated[j];
    }

    const bool solvedBlock = solveNormalImpulseLcp(
        cache.normalMatrix, normalOffset, count, solved);

    if (solvedBlock)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            ContactPoint& contact = manifold.points[i];
            const double delta = solved[i] - accumulated[i];
            contact.accumulatedNormalImpulse = static_cast<float>(solved[i]);
            if (std::fabs(delta) > 1e-8)
            {
                applyPairImpulse(
                    bodyA, bodyB,
                    cache.armsA[i], cache.armsB[i],
                    cache.inverseInertiaA, cache.inverseInertiaB,
                    cache.normals[i] * static_cast<float>(delta));
                maximumImpulseChange = std::max(
                    maximumImpulseChange, std::fabs(static_cast<float>(delta)));
            }
        }
    }
    else
    {
        // Fallback para manifolds degenerados/singularidades numéricas.
        for (std::size_t i = 0; i < count; ++i)
        {
            ContactPoint& contact = manifold.points[i];
            const glm::vec3 relativeVelocity =
                velocityAtPoint(bodyB, pointOnB(contact)) -
                velocityAtPoint(bodyA, pointOnA(contact));
            const float normalVelocity = glm::dot(relativeVelocity, cache.normals[i]);
            const float armDenominator = effectiveMassDenominator(
                bodyA, bodyB, cache.armsA[i], cache.armsB[i],
                cache.normals[i], cache.inverseInertiaA, cache.inverseInertiaB);
            if (armDenominator <= 1e-8f)
                continue;

            const float delta =
                (contact.restitutionBias - normalVelocity) / armDenominator;
            const float previous = contact.accumulatedNormalImpulse;
            contact.accumulatedNormalImpulse = std::max(0.0f, previous + delta);
            const float applied = contact.accumulatedNormalImpulse - previous;
            if (std::fabs(applied) > 1e-8f)
            {
                applyPairImpulse(
                    bodyA, bodyB,
                    cache.armsA[i], cache.armsB[i],
                    cache.inverseInertiaA, cache.inverseInertiaB,
                    cache.normals[i] * applied);
                maximumImpulseChange = std::max(
                    maximumImpulseChange, std::fabs(applied));
            }
        }
    }

    // Atrito tangencial acumulado por ponto, reutilizando braços e tensores.
    for (std::size_t i = 0; i < count; ++i)
    {
        ContactPoint& contact = manifold.points[i];
        if (contact.accumulatedNormalImpulse <= 0.0f)
            continue;

        const glm::vec3 relativeVelocity =
            velocityAtPoint(bodyB, pointOnB(contact)) -
            velocityAtPoint(bodyA, pointOnA(contact));
        const glm::vec3& normal = cache.normals[i];
        const glm::vec3 tangentVelocity = relativeVelocity -
            glm::dot(relativeVelocity, normal) * normal;
        const float tangentSpeed = glm::length(tangentVelocity);
        if (tangentSpeed <= 1e-7f)
            continue;

        const glm::vec3 tangent = tangentVelocity / tangentSpeed;
        const float tangentDenominator = effectiveMassDenominator(
            bodyA, bodyB, cache.armsA[i], cache.armsB[i], tangent,
            cache.inverseInertiaA, cache.inverseInertiaB);
        if (tangentDenominator <= 1e-8f)
            continue;

        const float tangentImpulseDelta =
            -glm::dot(relativeVelocity, tangent) / tangentDenominator;
        glm::vec3 candidateTangentImpulse =
            contact.accumulatedTangentImpulse + tangent * tangentImpulseDelta;
        const float candidateLength = glm::length(candidateTangentImpulse);
        const float staticLimit =
            clampFriction(material.staticFriction) * contact.accumulatedNormalImpulse;
        const float dynamicLimit =
            clampFriction(material.dynamicFriction) * contact.accumulatedNormalImpulse;
        if (candidateLength > staticLimit)
        {
            if (candidateLength > 1e-8f)
                candidateTangentImpulse *= dynamicLimit / candidateLength;
            else
                candidateTangentImpulse = glm::vec3(0.0f);
        }

        const glm::vec3 appliedTangentImpulse =
            candidateTangentImpulse - contact.accumulatedTangentImpulse;
        contact.accumulatedTangentImpulse = candidateTangentImpulse;
        if (glm::length2(appliedTangentImpulse) > 1e-16f)
        {
            applyPairImpulse(
                bodyA, bodyB,
                cache.armsA[i], cache.armsB[i],
                cache.inverseInertiaA, cache.inverseInertiaB,
                appliedTangentImpulse);
            maximumImpulseChange = std::max(
                maximumImpulseChange, glm::length(appliedTangentImpulse));
        }
    }

    return maximumImpulseChange;
}

float solveContactVelocity(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material)
{
    ContactSolverCache cache;
    prepareContactSolverCache(bodyA, bodyB, manifold, cache);
    return solveContactVelocity(bodyA, bodyB, manifold, material, cache);
}

void correctContactPosition(
    RigidBody* bodyA,
    RigidBody* bodyB,
    const ContactManifold& manifold,
    float correctionPercentage)
{
    if (manifold.empty())
        return;

    const float invMassA = inverseMass(bodyA);
    const float invMassB = inverseMass(bodyB);
    const float inverseMassSum = invMassA + invMassB;
    if (inverseMassSum <= 1e-8f)
        return;

    // Um manifold de malha pode conter normais diferentes. Em vez de usar
    // somente a normal do primeiro ponto, calculamos a direção média ponderada
    // pela penetração. Para contatos coplanares o resultado permanece idêntico.
    glm::vec3 weightedNormal(0.0f);
    float totalWeight = 0.0f;
    float maximumPenetration = 0.0f;
    for (std::size_t i = 0; i < manifold.pointCount; ++i)
    {
        const ContactPoint& point = manifold.points[i];
        const float depth = std::max(point.penetration - kPenetrationSlop, 0.0f);
        if (depth <= 0.0f)
            continue;
        const float normalLength = glm::length(point.normal);
        if (normalLength <= 1e-7f || !std::isfinite(normalLength))
            continue;
        weightedNormal += (point.normal / normalLength) * depth;
        totalWeight += depth;
        maximumPenetration = std::max(maximumPenetration, depth);
    }

    if (totalWeight <= 1e-8f || maximumPenetration <= 0.0f)
        return;

    const glm::vec3 averageNormal = weightedNormal / totalWeight;
    const float directionLength = glm::length(averageNormal);
    if (directionLength <= 1e-6f || !std::isfinite(directionLength))
        return;

    const glm::vec3 correction =
        (averageNormal / directionLength) * maximumPenetration *
        std::clamp(correctionPercentage, 0.0f, 1.0f);
    if (bodyA && bodyA->transform && invMassA > 0.0f)
        bodyA->transform->translate(-correction * (invMassA / inverseMassSum));
    if (bodyB && bodyB->transform && invMassB > 0.0f)
        bodyB->transform->translate(correction * (invMassB / inverseMassSum));
}

void resolveBodySurfaceContact(
    RigidBody& body,
    const ContactPoint& contact,
    const PhysicsMaterial& material)
{
    ContactManifold manifold;
    manifold.points[0] = contact;
    manifold.pointCount = 1;
    prepareContactManifold(nullptr, &body, manifold, material);
    ContactSolverCache cache;
    prepareContactSolverCache(nullptr, &body, manifold, cache);
    for (int iteration = 0; iteration < 8; ++iteration)
        solveContactVelocity(nullptr, &body, manifold, material, cache);
    correctContactPosition(nullptr, &body, manifold, 1.0f);
}

void resolveBodyPairContact(
    RigidBody& bodyA,
    RigidBody& bodyB,
    const ContactPoint& contact,
    const PhysicsMaterial& material)
{
    ContactManifold manifold;
    manifold.points[0] = contact;
    manifold.pointCount = 1;
    prepareContactManifold(&bodyA, &bodyB, manifold, material);
    ContactSolverCache cache;
    prepareContactSolverCache(&bodyA, &bodyB, manifold, cache);
    for (int iteration = 0; iteration < 8; ++iteration)
        solveContactVelocity(&bodyA, &bodyB, manifold, material, cache);
    correctContactPosition(&bodyA, &bodyB, manifold, 1.0f);
}
} // namespace rotor::physics::solver
