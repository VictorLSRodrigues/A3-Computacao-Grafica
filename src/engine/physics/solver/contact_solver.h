#pragma once

#include <glm/glm.hpp>

#include "engine/physics/physics_types.h"

namespace rotor::physics::solver
{
PhysicsMaterial combineMaterials(
    const PhysicsMaterial& a,
    const PhysicsMaterial& b);

// Prepara os dados iniciais (incluindo restituição) antes das iterações.
void prepareContactManifold(
    RigidBody* bodyA,
    RigidBody* bodyB,
    ContactManifold& manifold,
    const PhysicsMaterial& material);

// Resolve os impulsos normais do manifold como um pequeno sistema acoplado
// (até quatro pontos) e, em seguida, o atrito acumulado por ponto.
void solveContactVelocity(
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
