#include "engine/physics/mesh_collider.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <unordered_map>

namespace rotor::physics
{
namespace
{
struct QuantizedPosition
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t z = 0;

    bool operator==(const QuantizedPosition& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct QuantizedPositionHash
{
    std::size_t operator()(const QuantizedPosition& value) const
    {
        const auto mix = [](std::uint64_t x)
        {
            x ^= x >> 30;
            x *= 0xbf58476d1ce4e5b9ULL;
            x ^= x >> 27;
            x *= 0x94d049bb133111ebULL;
            x ^= x >> 31;
            return x;
        };

        const std::uint64_t x = mix(static_cast<std::uint64_t>(value.x));
        const std::uint64_t y = mix(static_cast<std::uint64_t>(value.y));
        const std::uint64_t z = mix(static_cast<std::uint64_t>(value.z));
        return static_cast<std::size_t>(x ^ (y << 1) ^ (z << 7));
    }
};

bool isFinite(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}

QuantizedPosition quantize(const glm::vec3& value)
{
    constexpr double scale = 1000000.0; // tolerância de unificação: 1 micrômetro
    return {
        static_cast<std::int64_t>(std::llround(value.x * scale)),
        static_cast<std::int64_t>(std::llround(value.y * scale)),
        static_cast<std::int64_t>(std::llround(value.z * scale))};
}


} // namespace

bool buildTriangleMeshColliderData(
    const std::vector<glm::vec3>& sourceVertices,
    const std::vector<unsigned int>& sourceTriangleIndices,
    TriangleMeshColliderData& result)
{
    result = TriangleMeshColliderData{};
    if (sourceVertices.size() < 3)
        return false;

    std::vector<unsigned int> inputIndices = sourceTriangleIndices;
    if (inputIndices.empty())
    {
        if (sourceVertices.size() % 3 != 0)
            return false;
        inputIndices.resize(sourceVertices.size());
        for (std::size_t i = 0; i < inputIndices.size(); ++i)
            inputIndices[i] = static_cast<unsigned int>(i);
    }

    if (inputIndices.size() < 3 || inputIndices.size() % 3 != 0)
        return false;

    std::unordered_map<QuantizedPosition, unsigned int, QuantizedPositionHash> uniqueMap;
    uniqueMap.reserve(sourceVertices.size());

    std::vector<unsigned int> remap(sourceVertices.size(), 0);
    result.vertices.reserve(sourceVertices.size());

    for (std::size_t i = 0; i < sourceVertices.size(); ++i)
    {
        const glm::vec3& position = sourceVertices[i];
        if (!isFinite(position))
            return false;

        const QuantizedPosition key = quantize(position);
        const auto found = uniqueMap.find(key);
        if (found != uniqueMap.end())
        {
            remap[i] = found->second;
            continue;
        }

        const unsigned int newIndex = static_cast<unsigned int>(result.vertices.size());
        result.vertices.push_back(position);
        uniqueMap.emplace(key, newIndex);
        remap[i] = newIndex;
    }

    result.triangleIndices.reserve(inputIndices.size());
    double totalArea = 0.0;
    glm::dvec3 weightedCentroid(0.0);

    for (std::size_t i = 0; i + 2 < inputIndices.size(); i += 3)
    {
        const unsigned int ia = inputIndices[i];
        const unsigned int ib = inputIndices[i + 1];
        const unsigned int ic = inputIndices[i + 2];
        if (ia >= remap.size() || ib >= remap.size() || ic >= remap.size())
        {
            result = TriangleMeshColliderData{};
            return false;
        }

        const unsigned int aIndex = remap[ia];
        const unsigned int bIndex = remap[ib];
        const unsigned int cIndex = remap[ic];
        if (aIndex == bIndex || bIndex == cIndex || cIndex == aIndex)
            continue;

        const glm::vec3& a = result.vertices[aIndex];
        const glm::vec3& b = result.vertices[bIndex];
        const glm::vec3& c = result.vertices[cIndex];
        const float twiceArea = glm::length(glm::cross(b - a, c - a));
        if (!std::isfinite(twiceArea) || twiceArea <= 1e-9f)
            continue;

        const double area = static_cast<double>(twiceArea) * 0.5;
        const glm::dvec3 centroid =
            (glm::dvec3(a) + glm::dvec3(b) + glm::dvec3(c)) / 3.0;
        totalArea += area;
        weightedCentroid += centroid * area;

        result.triangleIndices.push_back(aIndex);
        result.triangleIndices.push_back(bIndex);
        result.triangleIndices.push_back(cIndex);
    }

    if (totalArea <= 1e-8 || result.triangleIndices.size() < 3)
    {
        result = TriangleMeshColliderData{};
        return false;
    }

    const glm::dvec3 centerOfMass = weightedCentroid / totalArea;
    result.localCenterOfMass = glm::vec3(centerOfMass);
    result.surfaceArea = static_cast<float>(totalArea);

    // Covariância de uma distribuição superficial uniforme. Para uma malha
    // aberta, trata-se de uma aproximação de massa, não de um sólido volumétrico.
    glm::dmat3 covariance(0.0);
    for (std::size_t i = 0; i + 2 < result.triangleIndices.size(); i += 3)
    {
        const glm::vec3& a = result.vertices[result.triangleIndices[i]];
        const glm::vec3& b = result.vertices[result.triangleIndices[i + 1]];
        const glm::vec3& c = result.vertices[result.triangleIndices[i + 2]];

        const double area = static_cast<double>(
            glm::length(glm::cross(b - a, c - a)) * 0.5f);
        if (area <= 1e-12)
            continue;

        const glm::dvec3 da = glm::dvec3(a) - centerOfMass;
        const glm::dvec3 db = glm::dvec3(b) - centerOfMass;
        const glm::dvec3 dc = glm::dvec3(c) - centerOfMass;
        const glm::dvec3 triangleCenter = (da + db + dc) / 3.0;

        covariance += area * glm::outerProduct(triangleCenter, triangleCenter);
        covariance += (area / 12.0) * (
            glm::outerProduct(da - triangleCenter, da - triangleCenter) +
            glm::outerProduct(db - triangleCenter, db - triangleCenter) +
            glm::outerProduct(dc - triangleCenter, dc - triangleCenter));
    }

    covariance /= totalArea;
    result.unitMassCovariance = glm::mat3(covariance);

    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());
    for (glm::vec3& vertex : result.vertices)
    {
        vertex -= result.localCenterOfMass;
        minimum = glm::min(minimum, vertex);
        maximum = glm::max(maximum, vertex);
    }
    result.localBoundsCenter = (minimum + maximum) * 0.5f;
    result.localHalfExtents = (maximum - minimum) * 0.5f;

    // Constrói uma BVH mediana sobre os centroides dos triângulos. Os bounds
    // permanecem no espaço local da malha, portanto rotação/posição/escala do
    // corpo não exigem reconstrução da hierarquia.
    const std::size_t triangleCount = result.triangleIndices.size() / 3;
    if (triangleCount > std::numeric_limits<std::uint32_t>::max())
    {
        result = TriangleMeshColliderData{};
        return false;
    }

    result.triangleOrder.resize(triangleCount);
    std::iota(result.triangleOrder.begin(), result.triangleOrder.end(), 0u);
    result.bvhNodes.reserve(triangleCount * 2);

    const auto triangleBounds = [&result](std::uint32_t triangle,
                                          glm::vec3& minBounds,
                                          glm::vec3& maxBounds,
                                          glm::vec3& centroid)
    {
        const std::size_t base = static_cast<std::size_t>(triangle) * 3;
        const glm::vec3& a = result.vertices[result.triangleIndices[base]];
        const glm::vec3& b = result.vertices[result.triangleIndices[base + 1]];
        const glm::vec3& c = result.vertices[result.triangleIndices[base + 2]];
        minBounds = glm::min(a, glm::min(b, c));
        maxBounds = glm::max(a, glm::max(b, c));
        centroid = (a + b + c) / 3.0f;
    };

    const auto buildNode = [&](auto&& self, std::uint32_t first,
                               std::uint32_t count) -> std::uint32_t
    {
        const std::uint32_t nodeIndex =
            static_cast<std::uint32_t>(result.bvhNodes.size());
        result.bvhNodes.emplace_back();

        glm::vec3 nodeMin(std::numeric_limits<float>::max());
        glm::vec3 nodeMax(std::numeric_limits<float>::lowest());
        glm::vec3 centroidMin(std::numeric_limits<float>::max());
        glm::vec3 centroidMax(std::numeric_limits<float>::lowest());
        for (std::uint32_t i = first; i < first + count; ++i)
        {
            glm::vec3 triMin, triMax, centroid;
            triangleBounds(result.triangleOrder[i], triMin, triMax, centroid);
            nodeMin = glm::min(nodeMin, triMin);
            nodeMax = glm::max(nodeMax, triMax);
            centroidMin = glm::min(centroidMin, centroid);
            centroidMax = glm::max(centroidMax, centroid);
        }

        result.bvhNodes[nodeIndex].boundsMin = nodeMin;
        result.bvhNodes[nodeIndex].boundsMax = nodeMax;
        constexpr std::uint32_t leafSize = 8;
        glm::vec3 centroidExtent = centroidMax - centroidMin;
        int splitAxis = 0;
        if (centroidExtent.y > centroidExtent.x)
            splitAxis = 1;
        if (centroidExtent.z > centroidExtent[splitAxis])
            splitAxis = 2;

        if (count <= leafSize)
        {
            result.bvhNodes[nodeIndex].firstTriangle = first;
            result.bvhNodes[nodeIndex].triangleCount = count;
            return nodeIndex;
        }

        // Se todos os centroides coincidirem, ainda dividimos por quantidade
        // usando o eixo de maior extensão dos bounds. Isso impede folhas enormes
        // em geometrias sobrepostas ou com centroides repetidos.
        if (centroidExtent[splitAxis] <= 1e-8f)
        {
            const glm::vec3 nodeExtent = nodeMax - nodeMin;
            splitAxis = 0;
            if (nodeExtent.y > nodeExtent.x)
                splitAxis = 1;
            if (nodeExtent.z > nodeExtent[splitAxis])
                splitAxis = 2;
        }

        const std::uint32_t middle = first + count / 2;
        std::nth_element(
            result.triangleOrder.begin() + first,
            result.triangleOrder.begin() + middle,
            result.triangleOrder.begin() + first + count,
            [&](std::uint32_t lhs, std::uint32_t rhs)
            {
                glm::vec3 aMin, aMax, aCenter;
                glm::vec3 bMin, bMax, bCenter;
                triangleBounds(lhs, aMin, aMax, aCenter);
                triangleBounds(rhs, bMin, bMax, bCenter);
                return aCenter[splitAxis] < bCenter[splitAxis];
            });

        const std::uint32_t left = self(self, first, middle - first);
        const std::uint32_t right = self(self, middle, first + count - middle);
        result.bvhNodes[nodeIndex].leftChild = left;
        result.bvhNodes[nodeIndex].rightChild = right;
        return nodeIndex;
    };

    buildNode(buildNode, 0u, static_cast<std::uint32_t>(triangleCount));

    if (!isFinite(result.localCenterOfMass) ||
        !isFinite(result.localBoundsCenter) ||
        !isFinite(result.localHalfExtents) ||
        !std::isfinite(result.surfaceArea))
    {
        result = TriangleMeshColliderData{};
        return false;
    }

    return result.valid();
}
} // namespace rotor::physics
