#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

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

struct MeshContactPositionKey
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t z = 0;

    bool operator==(const MeshContactPositionKey& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct MeshContactPositionKeyHash
{
    std::size_t operator()(const MeshContactPositionKey& value) const;
};

// Memória temporária reutilizável pelas consultas de malha. Mantê-la no
// PhysicsWorld evita alocações repetidas a cada coleta de contatos.
struct MeshCollisionCounters
{
    std::uint64_t contactQueries = 0;
    std::uint64_t supportQueries = 0;
    std::uint64_t bvhNodesVisited = 0;
    std::uint64_t bvhNodesRejected = 0;
    std::uint64_t supportBvhNodesVisited = 0;
    std::uint64_t supportBvhNodesRejected = 0;
    std::uint64_t candidateTriangles = 0;
    std::uint64_t trianglesTested = 0;
    std::uint64_t verticesTested = 0;
    std::uint64_t supportVerticesTested = 0;
};

struct MeshCollisionScratch
{
    // Contadores ativados somente durante uma janela de profiling.
    bool collectStats = false;
    MeshCollisionCounters counters;
    std::vector<std::uint32_t> traversalStack;
    std::vector<std::uint32_t> candidateTriangles;
    std::vector<std::uint32_t> vertexStamps;
    std::vector<glm::vec3> transformedVertices;
    std::vector<ContactPoint> candidates;
    std::vector<ContactPoint> uniqueCandidates;
    std::vector<std::size_t> selectedCandidates;
    std::unordered_map<MeshContactPositionKey, std::size_t,
                       MeshContactPositionKeyHash> contactIndices;
    std::vector<glm::vec3> polygon;
    std::vector<glm::vec3> polygonScratch;
    std::uint32_t generation = 0;
};

OrientedBox makeOrientedBox(
    const glm::vec3& center,
    const glm::quat& orientation,
    const glm::vec3& halfExtents);

float projectedRadius(const OrientedBox& box, const glm::vec3& axis);
glm::vec3 supportPoint(const OrientedBox& box, const glm::vec3& direction);
// Consultas de suporte usam a forma efetiva do colisor e sua Transform.
glm::vec3 supportPoint(
    const RigidBody& body,
    const glm::vec3& direction,
    MeshCollisionScratch* scratch = nullptr);
float supportRadius(
    const RigidBody& body,
    const glm::vec3& direction,
    MeshCollisionScratch* scratch = nullptr);

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

// Contatos usando os vértices da malha original contra superfícies estáticas.
// A consulta preserva a geometria visual, reduzindo o resultado a até quatro
// pontos para o solver.
bool triangleMeshPlaneContacts(
    const RigidBody& body,
    const glm::vec3& planeNormal,
    float planeOffset,
    ContactManifold& manifold,
    MeshCollisionScratch* scratch = nullptr);

bool triangleMeshOrientedBoxContacts(
    const RigidBody& body,
    const OrientedBox& box,
    ContactManifold& manifold,
    MeshCollisionScratch* scratch = nullptr);

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
