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
constexpr float kPenetrationSlop = 0.0001f;

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
    const ContactPoint& contact,
    const glm::vec3& impulseOnB)
{
    if (bodyA && bodyA->transform && bodyA->inverseMass > 0.0f)
    {
        const glm::vec3 armA = pointOnA(contact) - bodyA->transform->position();
        const glm::mat3 inverseA = inverseInertia(bodyA);
        bodyA->velocity -= impulseOnB * bodyA->inverseMass;
        bodyA->angularVelocity -= inverseA * glm::cross(armA, impulseOnB);
    }

    if (bodyB && bodyB->transform && bodyB->inverseMass > 0.0f)
    {
        const glm::vec3 armB = pointOnB(contact) - bodyB->transform->position();
        const glm::mat3 inverseB = inverseInertia(bodyB);
        bodyB->velocity += impulseOnB * bodyB->inverseMass;
        bodyB->angularVelocity += inverseB * glm::cross(armB, impulseOnB);
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

void solveContactVelocity(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material)
{
    if (manifold.empty())
        return;

    const glm::mat3 inverseA = inverseInertia(bodyA);
    const glm::mat3 inverseB = inverseInertia(bodyB);
    const std::size_t count = manifold.pointCount;

    glm::vec3 armsA[ContactManifold::MaxPoints]{};
    glm::vec3 armsB[ContactManifold::MaxPoints]{};
    glm::vec3 normals[ContactManifold::MaxPoints]{};
    double normalMatrix[ContactManifold::MaxPoints]
                       [ContactManifold::MaxPoints]{};
    double normalOffset[ContactManifold::MaxPoints]{};
    double accumulated[ContactManifold::MaxPoints]{};
    double solved[ContactManifold::MaxPoints]{};

    // Constrói a matriz de massa efetiva acoplada do manifold. Cada impulso
    // normal num ponto afeta a velocidade relativa em todos os outros pontos.
    for (std::size_t i = 0; i < count; ++i)
    {
        ContactPoint& contact = manifold.points[i];
        armsA[i] = bodyA && bodyA->transform
            ? pointOnA(contact) - bodyA->transform->position() : glm::vec3(0.0f);
        armsB[i] = bodyB && bodyB->transform
            ? pointOnB(contact) - bodyB->transform->position() : glm::vec3(0.0f);
        normals[i] = glm::normalize(contact.normal);
        accumulated[i] = contact.accumulatedNormalImpulse;

        const glm::vec3 relativeVelocity =
            velocityAtPoint(bodyB, pointOnB(contact)) -
            velocityAtPoint(bodyA, pointOnA(contact));
        normalOffset[i] = static_cast<double>(
            glm::dot(relativeVelocity, normals[i]) - contact.restitutionBias);
    }

    for (std::size_t i = 0; i < count; ++i)
    {
        const glm::vec3 angularArmA = glm::cross(armsA[i], normals[i]);
        const glm::vec3 angularArmB = glm::cross(armsB[i], normals[i]);
        for (std::size_t j = 0; j < count; ++j)
        {
            const glm::vec3 angularArmAj = glm::cross(armsA[j], normals[j]);
            const glm::vec3 angularArmBj = glm::cross(armsB[j], normals[j]);
            const float coupling =
                (inverseMass(bodyA) + inverseMass(bodyB)) *
                    glm::dot(normals[i], normals[j]) +
                glm::dot(angularArmA, inverseA * angularArmAj) +
                glm::dot(angularArmB, inverseB * angularArmBj);
            normalMatrix[i][j] = static_cast<double>(coupling);
            normalOffset[i] -= normalMatrix[i][j] * accumulated[j];
        }
    }

    // Manifolds pequenos (até quatro pontos) permitem resolver as cargas
    // normais simultaneamente por active-set, evitando que a ordem dos vértices
    // escolha artificialmente um pivô para contatos face-a-face.
    const bool solvedBlock = solveNormalImpulseLcp(
        normalMatrix, normalOffset, count, solved);

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
                    bodyA, bodyB, contact,
                    normals[i] * static_cast<float>(delta));
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
            const float normalVelocity = glm::dot(relativeVelocity, contact.normal);
            const float armDenominator = effectiveMassDenominator(
                bodyA, bodyB, armsA[i], armsB[i], contact.normal, inverseA, inverseB);
            if (armDenominator <= 1e-8f)
                continue;

            const float delta =
                (contact.restitutionBias - normalVelocity) / armDenominator;
            const float previous = contact.accumulatedNormalImpulse;
            contact.accumulatedNormalImpulse = std::max(0.0f, previous + delta);
            const float applied = contact.accumulatedNormalImpulse - previous;
            if (std::fabs(applied) > 1e-8f)
                applyPairImpulse(bodyA, bodyB, contact, contact.normal * applied);
        }
    }

    // Atrito tangencial é resolvido por ponto, limitado pelo cone de Coulomb.
    // O limite depende do impulso normal já encontrado para aquele contato.
    for (std::size_t i = 0; i < count; ++i)
    {
        ContactPoint& contact = manifold.points[i];
        if (contact.accumulatedNormalImpulse <= 0.0f)
            continue;

        const glm::vec3 relativeVelocity =
            velocityAtPoint(bodyB, pointOnB(contact)) -
            velocityAtPoint(bodyA, pointOnA(contact));
        const glm::vec3 normal = normals[i];
        const glm::vec3 tangentVelocity = relativeVelocity -
            glm::dot(relativeVelocity, normal) * normal;
        const float tangentSpeed = glm::length(tangentVelocity);
        if (tangentSpeed <= 1e-7f)
            continue;

        const glm::vec3 tangent = tangentVelocity / tangentSpeed;
        const float tangentDenominator = effectiveMassDenominator(
            bodyA, bodyB, armsA[i], armsB[i], tangent, inverseA, inverseB);
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
            applyPairImpulse(bodyA, bodyB, contact, appliedTangentImpulse);
    }

}

void correctContactPosition(
    RigidBody* bodyA,
    RigidBody* bodyB,
    const ContactManifold& manifold,
    float correctionPercentage)
{
    if (manifold.empty())
        return;

    float maximumPenetration = 0.0f;
    for (std::size_t i = 0; i < manifold.pointCount; ++i)
        maximumPenetration = std::max(
            maximumPenetration, manifold.points[i].penetration);

    const float correctionDepth = std::max(
        maximumPenetration - kPenetrationSlop, 0.0f) *
        std::clamp(correctionPercentage, 0.0f, 1.0f);
    if (correctionDepth <= 0.0f)
        return;

    const float invMassA = inverseMass(bodyA);
    const float invMassB = inverseMass(bodyB);
    const float inverseMassSum = invMassA + invMassB;
    if (inverseMassSum <= 1e-8f)
        return;

    const glm::vec3 normal = manifold.points[0].normal;
    if (bodyA && bodyA->transform && invMassA > 0.0f)
    {
        bodyA->transform->translate(
            -normal * correctionDepth * (invMassA / inverseMassSum));
    }
    if (bodyB && bodyB->transform && invMassB > 0.0f)
    {
        bodyB->transform->translate(
            normal * correctionDepth * (invMassB / inverseMassSum));
    }
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
    for (int iteration = 0; iteration < 8; ++iteration)
        solveContactVelocity(nullptr, &body, manifold, material);
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
    for (int iteration = 0; iteration < 8; ++iteration)
        solveContactVelocity(&bodyA, &bodyB, manifold, material);
    correctContactPosition(&bodyA, &bodyB, manifold, 1.0f);
}
} // namespace rotor::physics::solver
