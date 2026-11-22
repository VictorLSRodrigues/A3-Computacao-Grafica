#include "geometry.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
    // Calcula o maior afastamento dos vértices em relação à origem.
    float calculateBoundingRadius(
        const std::vector<float> &vertices,
        int stride)
    {
        if (stride < 3)
            return 0.0f;

        float radius = 0.0f;

        for (size_t i = 0; i < vertices.size(); i += stride)
        {
            glm::vec3 position(
                vertices[i],
                vertices[i + 1],
                vertices[i + 2]);

            radius = std::max(radius, glm::length(position));
        }

        return radius;
    }

    BoundingBox calculateBoundingBox(
        const std::vector<Vertex> &vertices)
    {
        BoundingBox bounds;

        for (const Vertex &vertex : vertices)
        {
            bounds.expand(vertex.position);
        }

        return bounds;
    }

    BoundingBox calculateBoundingBox(
        const std::vector<float> &vertices,
        int stride)
    {
        BoundingBox bounds;

        if (stride < 3)
            return bounds;

        for (size_t i = 0; i + 2 < vertices.size(); i += stride)
        {
            glm::vec3 position(
                vertices[i],
                vertices[i + 1],
                vertices[i + 2]);

            bounds.expand(position);
        }

        return bounds;
    }

    float calculateBoundingRadius(
        const std::vector<Vertex> &vertices)
    {
        float radius = 0.0f;

        for (const Vertex &vertex : vertices)
        {
            radius = std::max(
                radius,
                glm::length(vertex.position));
        }

        return radius;
    }
}

// Calcula o número de valores por vértice, conforme o layout informado.
int Geometry::computeStride(
    int componentCount,
    const std::vector<int> &layout)
{
    if (layout.empty())
        return (componentCount >= 1 && componentCount <= 4)
                   ? componentCount
                   : 0;

    int stride = 0;

    for (int c : layout)
    {
        if (c < 1 || c > 4)
            return 0;

        stride += c;
    }

    return stride;
}

// Guarda uma geometria sem índices.
bool Geometry::create(
    const std::vector<float> &vertices,
    int componentCount,
    PrimitiveTopology topology,
    const std::vector<int> &layout)
{
    const int stride = computeStride(componentCount, layout);

    if (stride == 0)
    {
        std::cout << "Geometry: layout inválido." << std::endl;
        return false;
    }

    const size_t perVertex = static_cast<size_t>(stride);

    if (vertices.empty() || vertices.size() % perVertex != 0)
    {
        std::cout << "Geometry: vertices incompatíveis." << std::endl;
        return false;
    }

    const size_t totalVertices = vertices.size() / perVertex;

    if (totalVertices > static_cast<size_t>(
                            std::numeric_limits<int>::max()))
    {
        std::cout << "Geometry: quantidade de vertices muito grande." << std::endl;
        return false;
    }

    // Mantém uma cópia dos dados para consulta e desenho.
    vertexData = vertices;
    attributeSizes = layout.empty() ? std::vector<int>{componentCount} : layout;
    vertexFormat = VertexFormat::Raw;
    boundingRadius = calculateBoundingRadius(vertices, stride);
    boundingBox = calculateBoundingBox(vertices, stride);
    structuredVertices.clear();
    indexData.clear();
    faces.clear();

    this->topology = topology;

    return true;
}

// Guarda uma geometria cujos triângulos referenciam vértices por índice.
bool Geometry::createIndexed(
    const std::vector<float> &vertices,
    const std::vector<unsigned int> &indices,
    int componentCount,
    PrimitiveTopology topology,
    const std::vector<int> &layout)
{
    const int stride = computeStride(componentCount, layout);

    if (stride == 0)
    {
        std::cout << "Geometry: layout inválido." << std::endl;
        return false;
    }

    const size_t perVertex = static_cast<size_t>(stride);

    if (vertices.empty() || vertices.size() % perVertex != 0)
    {
        std::cout << "Geometry: vertices incompatíveis." << std::endl;
        return false;
    }

    if (indices.empty())
    {
        std::cout << "Geometry: lista de indices vazia." << std::endl;
        return false;
    }

    const size_t totalVertices = vertices.size() / perVertex;

    if (totalVertices > static_cast<size_t>(
                            std::numeric_limits<int>::max()) ||
        indices.size() > static_cast<size_t>(
                             std::numeric_limits<int>::max()))
    {
        std::cout << "Geometry: quantidade de dados muito grande." << std::endl;
        return false;
    }

    for (unsigned int index : indices)
    {
        if (index >= totalVertices)
        {
            std::cout << "Geometry: indice fora do intervalo." << std::endl;
            return false;
        }
    }

    // Mantém cópias dos vértices e índices para consulta.
    vertexData = vertices;
    attributeSizes = layout.empty() ? std::vector<int>{componentCount} : layout;
    vertexFormat = VertexFormat::Raw;
    boundingRadius = calculateBoundingRadius(vertices, stride);
    boundingBox = calculateBoundingBox(vertices, stride);
    structuredVertices.clear();
    indexData = indices;
    faces.clear();

    this->topology = topology;

    return true;
}

// Guarda vértices estruturados sem índices.
bool Geometry::create(
    const std::vector<Vertex> &vertices,
    PrimitiveTopology topology)
{
    if (vertices.empty() ||
        vertices.size() > static_cast<size_t>(
                              std::numeric_limits<int>::max()))
    {
        std::cout << "Geometry: vertices inválidos." << std::endl;
        return false;
    }

    // Mantém uma cópia dos vértices para consulta.
    structuredVertices = vertices;
    vertexFormat = VertexFormat::Structured;
    boundingRadius = calculateBoundingRadius(vertices);
    boundingBox = calculateBoundingBox(vertices);
    vertexData.clear();
    attributeSizes.clear();
    indexData.clear();

    // Reconstrói a topologia editável.
    faces.clear();

    if (topology == PrimitiveTopology::Triangles)
    {
        for (unsigned int i = 0; i + 2 < vertices.size(); i += 3)
        {
            Face face;
            face.vertexIndices = {i, i + 1, i + 2};

            faces.push_back(std::move(face));
        }
    }

    this->topology = topology;

    return true;
}

// Guarda vértices estruturados com índices.
bool Geometry::createIndexed(
    const std::vector<Vertex> &vertices,
    const std::vector<unsigned int> &indices,
    PrimitiveTopology topology)
{
    if (vertices.empty() || indices.empty())
    {
        std::cout << "Geometry: vertices ou indices vazios." << std::endl;
        return false;
    }

    if (vertices.size() > static_cast<size_t>(
                              std::numeric_limits<int>::max()) ||
        indices.size() > static_cast<size_t>(
                             std::numeric_limits<int>::max()))
    {
        std::cout << "Geometry: quantidade de dados muito grande." << std::endl;
        return false;
    }

    for (unsigned int index : indices)
    {
        if (index >= vertices.size())
        {
            std::cout << "Geometry: indice fora do intervalo." << std::endl;
            return false;
        }
    }

    // Mantém cópias dos vértices e índices para consulta.
    structuredVertices = vertices;
    vertexFormat = VertexFormat::Structured;
    boundingRadius = calculateBoundingRadius(vertices);
    boundingBox = calculateBoundingBox(vertices);
    vertexData.clear();
    attributeSizes.clear();
    indexData = indices;

    // Reconstrói as faces a partir dos índices.
    faces.clear();

    if (topology == PrimitiveTopology::Triangles)
    {
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            Face face;

            face.vertexIndices = {
                indices[i],
                indices[i + 1],
                indices[i + 2]};

            faces.push_back(std::move(face));
        }
    }

    this->topology = topology;

    return true;
}

bool Geometry::intersectRay(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection,
    unsigned int &hitFaceIndex,
    float &hitDistance) const
{
    if (structuredVertices.empty() || faces.empty())
        return false;

    const float directionLength = glm::length(rayDirection);

    if (directionLength < 0.000001f)
        return false;

    const glm::vec3 direction =
        rayDirection / directionLength;

    constexpr float epsilon = 0.000001f;

    float nearestDistance =
        std::numeric_limits<float>::max();

    bool foundIntersection = false;

    for (size_t i = 0; i < faces.size(); ++i)
    {
        const Face &face = faces[i];

        if (face.vertexIndices.size() != 3)
            continue;

        const unsigned int ia = face.vertexIndices[0];
        const unsigned int ib = face.vertexIndices[1];
        const unsigned int ic = face.vertexIndices[2];

        if (ia >= structuredVertices.size() ||
            ib >= structuredVertices.size() ||
            ic >= structuredVertices.size())
            continue;

        const glm::vec3 &v0 = structuredVertices[ia].position;
        const glm::vec3 &v1 = structuredVertices[ib].position;
        const glm::vec3 &v2 = structuredVertices[ic].position;

        const glm::vec3 edge1 = v1 - v0;
        const glm::vec3 edge2 = v2 - v0;

        const glm::vec3 p = glm::cross(direction, edge2);

        const float determinant = glm::dot(edge1, p);

        if (std::abs(determinant) < epsilon)
            continue;

        const float inverseDeterminant = 1.0f / determinant;

        const glm::vec3 t = rayOrigin - v0;

        const float u =
            glm::dot(t, p) * inverseDeterminant;

        if (u < 0.0f || u > 1.0f)
            continue;

        const glm::vec3 q = glm::cross(t, edge1);

        const float v =
            glm::dot(direction, q) * inverseDeterminant;

        if (v < 0.0f || u + v > 1.0f)
            continue;

        const float distance =
            glm::dot(edge2, q) * inverseDeterminant;

        if (distance < 0.0f)
            continue;

        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            hitFaceIndex = static_cast<unsigned int>(i);
            foundIntersection = true;
        }
    }

    if (!foundIntersection)
        return false;

    hitDistance = nearestDistance;

    return true;
}

// Recupera as posições dos vértices de uma face.
bool Geometry::getFacePositions(
    unsigned int faceIndex,
    glm::vec3 &a,
    glm::vec3 &b,
    glm::vec3 &c) const
{
    if (faceIndex >= faces.size())
        return false;

    const Face &face = faces[faceIndex];

    if (face.vertexIndices.size() != 3)
        return false;

    const unsigned int ia = face.vertexIndices[0];
    const unsigned int ib = face.vertexIndices[1];
    const unsigned int ic = face.vertexIndices[2];

    if (ia >= structuredVertices.size() ||
        ib >= structuredVertices.size() ||
        ic >= structuredVertices.size())
        return false;

    a = structuredVertices[ia].position;
    b = structuredVertices[ib].position;
    c = structuredVertices[ic].position;

    return true;
}

bool Geometry::beginExtrusion(unsigned int faceIndex)
{
    // Evita iniciar duas operações simultaneamente.
    if (extrusionInProgress)
        return false;

    // Verifica se a malha permite extrusão.
    if (!isIndexed() ||
        topology != PrimitiveTopology::Triangles ||
        structuredVertices.empty() ||
        indexData.empty())
        return false;

    if (faceIndex >= faces.size())
        return false;

    // Nesta primeira versão, somente triângulos.
    if (faces[faceIndex].vertexIndices.size() != 3)
        return false;

    // Guarda a geometria original.
    extrusionBaseVertices = structuredVertices;
    extrusionBaseIndices = indexData;
    extrusionBaseFaces = faces;

    // Guarda qual face será extrudada.
    extrusionFaceIndex = faceIndex;
    extrusionBaseHadExtrusion = extrusionApplied;
    extrusionInProgress = true;

    return true;
}

bool Geometry::updateExtrusion(float distance)
{
    if (!extrusionInProgress || !std::isfinite(distance))
        return false;

    if (extrusionFaceIndex >= extrusionBaseFaces.size())
        return false;

    // Sempre começa pela geometria original.
    std::vector<Vertex> vertices = extrusionBaseVertices;
    std::vector<unsigned int> indices = extrusionBaseIndices;

    const Face &face = extrusionBaseFaces[extrusionFaceIndex];

    if (face.vertexIndices.size() != 3)
        return false;

    const unsigned int ia = face.vertexIndices[0];
    const unsigned int ib = face.vertexIndices[1];
    const unsigned int ic = face.vertexIndices[2];

    if (ia >= vertices.size() ||
        ib >= vertices.size() ||
        ic >= vertices.size())
        return false;

    const glm::vec3 a = vertices[ia].position;
    const glm::vec3 b = vertices[ib].position;
    const glm::vec3 c = vertices[ic].position;

    // Remove a face original da superfície da malha.
    const size_t originalFaceOffset =
        static_cast<size_t>(extrusionFaceIndex) * 3;

    if (originalFaceOffset + 2 >= indices.size())
        return false;

    indices.erase(
        indices.begin() + originalFaceOffset,
        indices.begin() + originalFaceOffset + 3);

    // Calcula a normal da face.
    glm::vec3 normal = glm::cross(b - a, c - a);

    const float normalLength = glm::length(normal);

    if (normalLength < 0.000001f)
        return false;

    normal /= normalLength;

    // Distância zero restaura a geometria original.
    if (std::abs(distance) < 0.000001f)
    {
        const bool restored = createIndexed(
            extrusionBaseVertices,
            extrusionBaseIndices,
            PrimitiveTopology::Triangles);

        if (restored)
            extrusionApplied = extrusionBaseHadExtrusion;

        return restored;
    }

    // Serão adicionados 3 vértices da tampa e 12 das laterais.
    if (vertices.size() >
        static_cast<size_t>(
            std::numeric_limits<unsigned int>::max() - 15u))
        return false;

    const glm::vec3 offset = normal * distance;

    // Cria os três vértices da nova face.
    Vertex capVertices[3] =
        {
            vertices[ia],
            vertices[ib],
            vertices[ic]};

    for (Vertex &vertex : capVertices)
    {
        vertex.position += offset;

        // A tampa aponta para fora do sólido.
        vertex.normal = distance > 0.0f ? normal : -normal;
    }

    const unsigned int capStart =
        static_cast<unsigned int>(vertices.size());

    for (const Vertex &vertex : capVertices)
        vertices.push_back(vertex);

    // Cria a nova face superior.
    if (distance > 0.0f)
    {
        indices.insert(indices.end(), {capStart,
                                       capStart + 1,
                                       capStart + 2});

        // A nova tampa será a próxima face registrada.
        extrusionResultFaceIndex =
            static_cast<unsigned int>(indices.size() / 3);
    }
    else
    {
        indices.insert(indices.end(), {capStart,
                                       capStart + 2,
                                       capStart + 1});
    }

    // Constrói as três paredes laterais.
    for (unsigned int i = 0; i < 3; ++i)
    {
        const unsigned int next = (i + 1) % 3;

        Vertex side[4];

        if (distance > 0.0f)
        {
            side[0] = vertices[face.vertexIndices[i]];
            side[1] = vertices[face.vertexIndices[next]];
            side[2] = capVertices[next];
            side[3] = capVertices[i];
        }
        else
        {
            side[0] = vertices[face.vertexIndices[next]];
            side[1] = vertices[face.vertexIndices[i]];
            side[2] = capVertices[i];
            side[3] = capVertices[next];
        }

        // Normal própria para cada parede.
        glm::vec3 sideNormal = glm::cross(
            side[1].position - side[0].position,
            side[2].position - side[0].position);

        const float sideNormalLength = glm::length(sideNormal);

        if (sideNormalLength < 0.000001f)
            return false;

        sideNormal /= sideNormalLength;

        for (Vertex &vertex : side)
            vertex.normal = sideNormal;

        const unsigned int sideStart =
            static_cast<unsigned int>(vertices.size());

        // Cada parede recebe quatro vértices próprios.
        for (const Vertex &vertex : side)
            vertices.push_back(vertex);

        // Dois triângulos formam a parede quadrilateral.
        indices.insert(indices.end(), {sideStart,
                                       sideStart + 1,
                                       sideStart + 2,

                                       sideStart,
                                       sideStart + 2,
                                       sideStart + 3});
    }

    // Atualiza vértices, índices, faces e buffers da GPU.
    const bool updated = createIndexed(
        vertices,
        indices,
        PrimitiveTopology::Triangles);

    if (updated)
        extrusionApplied = true;

    return updated;
}

void Geometry::finishExtrusion()
{
    if (!extrusionInProgress)
        return;

    extrusionBaseVertices.clear();
    extrusionBaseIndices.clear();
    extrusionBaseFaces.clear();

    extrusionInProgress = false;
    extrusionFaceIndex = 0;
}

bool Geometry::restoreExtrudedGeometry(
    const std::vector<Vertex> &vertices,
    const std::vector<unsigned int> &indices)
{
    if (!createIndexed(
            vertices,
            indices,
            PrimitiveTopology::Triangles))
        return false;

    extrusionApplied = true;
    return true;
}

bool Geometry::cancelExtrusion()
{
    if (!extrusionInProgress)
        return false;

    // Restaura a geometria original.
    if (!createIndexed(
            extrusionBaseVertices,
            extrusionBaseIndices,
            PrimitiveTopology::Triangles))
        return false;

    faces = extrusionBaseFaces;
    extrusionApplied = extrusionBaseHadExtrusion;

    finishExtrusion();

    return true;
}

void Geometry::clear()
{
    topology = PrimitiveTopology::Triangles;
    vertexFormat = VertexFormat::None;

    vertexData.clear();
    attributeSizes.clear();
    structuredVertices.clear();
    indexData.clear();
    faces.clear();

    boundingRadius = 0.0f;
    boundingBox = BoundingBox{};

    extrusionBaseVertices.clear();
    extrusionBaseIndices.clear();
    extrusionBaseFaces.clear();
    extrusionFaceIndex = 0;
    extrusionInProgress = false;

    extrusionApplied = false;
    extrusionBaseHadExtrusion = false;
    extrusionResultFaceIndex = 0;
}

std::size_t Geometry::getVertexCount() const
{
    if (vertexFormat == VertexFormat::Structured)
        return structuredVertices.size();

    if (vertexFormat == VertexFormat::Raw)
    {
        int stride = 0;

        for (int size : attributeSizes)
            stride += size;

        return stride > 0 ? vertexData.size() / static_cast<size_t>(stride) : 0;
    }

    return 0;
}
