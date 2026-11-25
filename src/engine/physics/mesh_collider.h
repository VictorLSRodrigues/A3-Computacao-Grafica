#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace rotor::physics
{
struct TriangleMeshBvhNode
{
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    std::uint32_t firstTriangle = 0;
    std::uint32_t triangleCount = 0;
    std::uint32_t leftChild = 0;
    std::uint32_t rightChild = 0;

    bool isLeaf() const { return triangleCount != 0; }
};

// Geometria triangulada usada pela física. Os vértices são armazenados em
// coordenadas locais relativas ao centro de massa aproximado da malha.
// A geometria é independente de OpenGL e pode ser compartilhada pelos corpos.
struct TriangleMeshColliderData
{
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> triangleIndices;
    // BVH local estática: triangleOrder armazena IDs de triângulos (não índices
    // de vértices). A hierarquia é construída uma vez e reutilizada nas consultas.
    std::vector<TriangleMeshBvhNode> bvhNodes;
    std::vector<std::uint32_t> triangleOrder;
    std::vector<unsigned int> supportVertexIndices;

    // Centro de massa aproximado no espaço local original da origem do modelo.
    // Para malhas abertas, é o centróide ponderado pela área superficial.
    glm::vec3 localCenterOfMass{0.0f};
    // Centro da AABB relativo ao centro de massa; usado apenas como proxy
    // conservador nas interações entre dois corpos dinâmicos.
    glm::vec3 localBoundsCenter{0.0f};
    glm::vec3 localHalfExtents{0.0f};

    // Covariância de massa unitária, calculada sobre a superfície triangulada,
    // centrada no centro de massa. Permite estimar a inércia após aplicar escala.
    glm::mat3 unitMassCovariance{0.0f};

    float surfaceArea = 0.0f;

    bool valid() const
    {
        return !vertices.empty() && triangleIndices.size() >= 3 &&
            triangleIndices.size() % 3 == 0 && surfaceArea > 1e-8f;
    }
};

// Constrói uma representação física a partir dos vértices e índices da malha
// visual. Vértices coincidentes são unificados e triângulos degenerados removidos.
bool buildTriangleMeshColliderData(
    const std::vector<glm::vec3>& sourceVertices,
    const std::vector<unsigned int>& sourceTriangleIndices,
    TriangleMeshColliderData& result);
} // namespace rotor::physics
