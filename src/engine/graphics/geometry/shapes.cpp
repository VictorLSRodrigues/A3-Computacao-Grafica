#include "shapes.h"

#include <array>
#include <cmath>
#include <limits>

// 2D

std::vector<float> shapes::triangle(float side)
{
    float height = side * std::sqrt(3.0f) / 2.0f;

    // Posiciona a base e o topo para que o centro geométrico fique na origem.
    return {
        -side / 2.0f, -height / 3.0f, 0.0f,
        side / 2.0f, -height / 3.0f, 0.0f,
        0.0f, 2.0f * height / 3.0f, 0.0f};
}

std::vector<float> shapes::square()
{
    return {
        -0.15f, -0.15f,
        0.15f, -0.15f,
        0.15f, 0.15f,

        -0.15f, -0.15f,
        0.15f, 0.15f,
        -0.15f, 0.15f};
}

std::vector<float> shapes::rectangle()
{
    return {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};
}

std::vector<unsigned int> shapes::rectangleIndices()
{
    return {0, 1, 2, 0, 2, 3};
}

// 3D

// Cada vértice da pirâmide tem 11 valores: posição (3), cor (3), UV (2) e normal (3).
// A base usa a imagem inteira. Cada face lateral usa um triângulo da imagem.

std::vector<float> shapes::pyramid()
{
    const float n = 0.894427f;
    const float m = 0.447214f;

    return {
        // Base
        -0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,

        // Face esquerda
        -0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, -n, m, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, -n, m, 0.0f,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, -n, m, 0.0f,

        // Face de trás
        0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, 0.0f, m, -n,
        -0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, 0.0f, m, -n,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, 0.0f, m, -n,

        // Face direita
        0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, n, m, 0.0f,
        0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, n, m, 0.0f,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, n, m, 0.0f,

        // Face da frente
        -0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, 0.0f, m, n,
        0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, 0.0f, m, n,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, 0.0f, m, n};
}

std::vector<unsigned int> shapes::pyramidIndices()
{
    return {
        0, 1, 2, 0, 2, 3, // base
        4, 5, 6,          // esquerda
        7, 8, 9,          // trás
        10, 11, 12,       // direita
        13, 14, 15        // frente
    };
}

bool shapes::gridPlane(
    float width,
    float depth,
    unsigned int segmentsX,
    unsigned int segmentsZ,
    std::vector<Vertex> &vertices,
    std::vector<unsigned int> &indices)
{
    vertices.clear();
    indices.clear();

    if (!std::isfinite(width) || !std::isfinite(depth) ||
        width <= 0.0f || depth <= 0.0f ||
        segmentsX == 0 || segmentsZ == 0)
        return false;

    const std::size_t columns = static_cast<std::size_t>(segmentsX) + 1;
    const std::size_t rows = static_cast<std::size_t>(segmentsZ) + 1;
    const std::size_t maxCount = static_cast<std::size_t>(
        std::numeric_limits<int>::max());

    if (segmentsX >= maxCount || segmentsZ >= maxCount)
        return false;

    if (columns > maxCount / rows)
        return false;

    const std::size_t vertexCount = columns * rows;
    const std::size_t quadCount =
        static_cast<std::size_t>(segmentsX) * segmentsZ;

    if (quadCount > maxCount / 6 || vertexCount > maxCount)
        return false;

    vertices.reserve(vertexCount);
    indices.reserve(quadCount * 6);

    for (unsigned int z = 0; z <= segmentsZ; ++z)
    {
        const float v = static_cast<float>(z) /
            static_cast<float>(segmentsZ);
        const float positionZ = (v - 0.5f) * depth;

        for (unsigned int x = 0; x <= segmentsX; ++x)
        {
            const float u = static_cast<float>(x) /
                static_cast<float>(segmentsX);
            const float positionX = (u - 0.5f) * width;

            vertices.push_back({
                {positionX, 0.0f, positionZ},
                {1.0f, 1.0f, 1.0f},
                {u, v},
                {0.0f, 1.0f, 0.0f},
                {1.0f, 0.0f, 0.0f, 1.0f}});
        }
    }

    // A ordem dos índices mantém as duas faces de cada célula voltadas para +Y.
    for (unsigned int z = 0; z < segmentsZ; ++z)
    {
        for (unsigned int x = 0; x < segmentsX; ++x)
        {
            const unsigned int a = static_cast<unsigned int>(
                static_cast<std::size_t>(z) * columns + x);
            const unsigned int b = a + 1;
            const unsigned int d = static_cast<unsigned int>(
                static_cast<std::size_t>(z + 1) * columns + x);
            const unsigned int c = d + 1;

            indices.insert(indices.end(), {a, d, c, a, c, b});
        }
    }

    return true;
}

bool shapes::box(
    float width,
    float height,
    float depth,
    std::vector<Vertex> &vertices,
    std::vector<unsigned int> &indices)
{
    vertices.clear();
    indices.clear();

    if (!std::isfinite(width) || !std::isfinite(height) ||
        !std::isfinite(depth) ||
        width <= 0.0f || height <= 0.0f || depth <= 0.0f)
        return false;

    struct FaceFrame
    {
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    const std::array<FaceFrame, 6> faces{{
        {{ 0.0f,  0.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f,  0.0f}},
        {{ 0.0f,  0.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f,  0.0f}},
        {{ 1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f,  0.0f}},
        {{-1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f,  0.0f}},
        {{ 0.0f,  1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.0f, -1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f,  1.0f}}
    }};

    const glm::vec3 halfExtents(width * 0.5f, height * 0.5f, depth * 0.5f);
    const std::array<glm::vec2, 4> uvCoordinates{{
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    }};
    const std::array<glm::vec2, 4> corners{{
        {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}
    }};

    const auto extentAlong = [&halfExtents](const glm::vec3 &axis)
    {
        return std::abs(axis.x) * halfExtents.x +
               std::abs(axis.y) * halfExtents.y +
               std::abs(axis.z) * halfExtents.z;
    };

    vertices.reserve(24);
    indices.reserve(36);

    for (const FaceFrame &face : faces)
    {
        const glm::vec3 center = face.normal * extentAlong(face.normal);
        const float tangentExtent = extentAlong(face.tangent);
        const float bitangentExtent = extentAlong(face.bitangent);
        const unsigned int firstVertex =
            static_cast<unsigned int>(vertices.size());

        for (std::size_t i = 0; i < corners.size(); ++i)
        {
            const glm::vec2 &corner = corners[i];
            const glm::vec3 position = center +
                face.tangent * (corner.x * 2.0f * tangentExtent) +
                face.bitangent * (corner.y * 2.0f * bitangentExtent);

            vertices.push_back({
                position,
                glm::vec3(1.0f),
                uvCoordinates[i],
                face.normal,
                glm::vec4(face.tangent, 1.0f)
            });
        }

        indices.insert(indices.end(), {
            firstVertex, firstVertex + 1, firstVertex + 2,
            firstVertex, firstVertex + 2, firstVertex + 3
        });
    }

    return true;
}

bool shapes::sphere(
    float radius,
    unsigned int sectors,
    unsigned int stacks,
    std::vector<Vertex> &vertices,
    std::vector<unsigned int> &indices)
{
    vertices.clear();
    indices.clear();

    if (!std::isfinite(radius) || radius <= 0.0f ||
        sectors < 3 || stacks < 2)
        return false;

    const std::size_t maxCount = static_cast<std::size_t>(
        std::numeric_limits<int>::max());

    if (sectors >= maxCount || stacks >= maxCount)
        return false;

    const std::size_t columns = static_cast<std::size_t>(sectors) + 1;
    const std::size_t rows = static_cast<std::size_t>(stacks) + 1;

    if (columns > maxCount / rows)
        return false;

    const std::size_t vertexCount = columns * rows;

    // Cada faixa interna gera dois triângulos por setor; as faixas dos
    // polos geram apenas um, evitando triângulos degenerados.
    const std::size_t bandCount = static_cast<std::size_t>(stacks) - 1;
    if (static_cast<std::size_t>(sectors) > maxCount / 6 / bandCount)
        return false;

    vertices.reserve(vertexCount);
    indices.reserve(
        static_cast<std::size_t>(sectors) * bandCount * 6);

    constexpr float pi = 3.14159265358979323846f;
    constexpr float twoPi = 2.0f * pi;

    for (unsigned int stack = 0; stack <= stacks; ++stack)
    {
        const float v = static_cast<float>(stack) /
            static_cast<float>(stacks);
        const float phi = v * pi;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (unsigned int sector = 0; sector <= sectors; ++sector)
        {
            const float u = static_cast<float>(sector) /
                static_cast<float>(sectors);
            const float theta = u * twoPi;
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            const glm::vec3 normal(
                sinPhi * cosTheta,
                cosPhi,
                sinPhi * sinTheta);
            const glm::vec3 tangent(-sinTheta, 0.0f, cosTheta);

            vertices.push_back({
                radius * normal,
                glm::vec3(1.0f),
                glm::vec2(u, 1.0f - v),
                normal,
                glm::vec4(tangent, 1.0f)});
        }
    }

    // O primeiro e o último anel coincidem nos polos. A costura UV também
    // duplica os vértices da borda para manter U=0 e U=1 independentes.
    for (unsigned int stack = 0; stack < stacks; ++stack)
    {
        for (unsigned int sector = 0; sector < sectors; ++sector)
        {
            const unsigned int a = static_cast<unsigned int>(
                static_cast<std::size_t>(stack) * columns + sector);
            const unsigned int b = a + 1;
            const unsigned int d = static_cast<unsigned int>(
                static_cast<std::size_t>(stack + 1) * columns + sector);
            const unsigned int c = d + 1;

            // A ordem mantém as faces voltadas para fora da esfera.
            if (stack > 0)
                indices.insert(indices.end(), {a, b, c});

            if (stack + 1 < stacks)
                indices.insert(indices.end(), {a, c, d});
        }
    }

    return true;
}

// Cubo
std::vector<float> shapes::cube()
{
    return {
        // Frente
        -0.5f, -0.5f, 0.5f, // 0
        0.5f, -0.5f, 0.5f,  // 1
        0.5f, 0.5f, 0.5f,   // 2
        -0.5f, 0.5f, 0.5f,  // 3

        // Fundo
        -0.5f, -0.5f, -0.5f, // 4
        0.5f, -0.5f, -0.5f,  // 5
        0.5f, 0.5f, -0.5f,   // 6
        -0.5f, 0.5f, -0.5f   // 7
    };
}

std::vector<unsigned int> shapes::cubeIndices()
{
    return {
        // Frente
        0, 1, 2, 0, 2, 3,

        // Fundo
        5, 4, 7, 5, 7, 6,

        // Direita
        1, 5, 6, 1, 6, 2,

        // Esquerda
        4, 0, 3, 4, 3, 7,

        // Cima
        3, 2, 6, 3, 6, 7,

        // Baixo
        4, 5, 1, 4, 1, 0};
}
