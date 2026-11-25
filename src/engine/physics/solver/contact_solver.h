#pragma once

#include <glm/glm.hpp>
#include <cstddef>

#include "engine/physics/physics_types.h"

namespace rotor::physics::solver
{
struct ContactSolverCache
{
    glm::mat3 inverseInertiaA{0.0f};
    glm::mat3 inverseInertiaB{0.0f};
    glm::vec3 armsA[ContactManifold::MaxPoints]{};
    glm::vec3 armsB[ContactManifold::MaxPoints]{};
    glm::vec3 normals[ContactManifold::MaxPoints]{};
    double normalMatrix[ContactManifold::MaxPoints]
                       [ContactManifold::MaxPoints]{};
    float inverseMassA = 0.0f;
    float inverseMassB = 0.0f;
    std::size_t pointCount = 0;
};

PhysicsMaterial combineMaterials(
    const PhysicsMaterial& a,
    const PhysicsMaterial& b);

// Prepara os dados iniciais (incluindo restituição) antes das iterações.
void prepareContactManifold(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material);

// Pré-calcula braços, inércias e a matriz de massa efetiva. Esses valores não
// mudam durante as iterações de velocidade do mesmo subpasso.
void prepareContactSolverCache(
    RigidBody* bodyA,
    RigidBody* bodyB,
    const ContactManifold& manifold,
    ContactSolverCache& cache);

// Resolve os impulsos reutilizando o cache preparado para o manifold.
float solveContactVelocity(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material,
    const ContactSolverCache& cache);

// Versão de conveniência para chamadas externas e testes; prepara cache local.
float solveContactVelocity(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material);

// Correção posicional translacional, separada da resolução de velocidade.
void correctContactPosition(
    RigidBody* bodyA,
    RigidBody* bodyB,
    const ContactManifold& manifold,
    float correctionPercentage = 0.2f);

// Wrappers simples mantidos para consultas/testes que resolvem um único ponto.
void resolveBodySurfaceContact(
    RigidBody& body,
    const ContactPoint& contact,
    const PhysicsMaterial& material);

void resolveBodyPairContact(
    RigidBody& bodyA,
    RigidBody& bodyB,
    const ContactPoint& contact,
    const PhysicsMaterial& material);
} // namespace rotor::physics::solver
