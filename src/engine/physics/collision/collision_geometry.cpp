#include "engine/physics/collision/collision_geometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <glm/gtx/norm.hpp>
#include <vector>

namespace rotor::physics::collision
{
std::size_t MeshContactPositionKeyHash::operator()(
    const MeshContactPositionKey& value) const
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

namespace
{
constexpr float kBowlLipStart = 0.78f;
constexpr float kContactTolerance = 1e-4f;

std::uint32_t beginVertexStampGeneration(
    MeshCollisionScratch& scratch,
    std::size_t vertexCount);

void evaluateBowlProfile(
    float radius,
    float depth,
    float lipHeight,
    float radialDistance,
    float& height,
    float& slope,
    float& curvature)
{
    const float t = std::clamp(radialDistance / radius, 0.0f, 1.0f);
    const float inverseRadius = 1.0f / radius;
    const float lipWidth = 1.0f - kBowlLipStart;

    height = depth * t * t;
    slope = 2.0f * depth * radialDistance * inverseRadius * inverseRadius;
    curvature = 2.0f * depth * inverseRadius * inverseRadius;

    if (t > kBowlLipStart && lipHeight > 0.0f)
    {
        const float u = (t - kBowlLipStart) / lipWidth;
        const float u2 = u * u;
        const float u3 = u2 * u;
        height += lipHeight * u2 * u2;
        slope += 4.0f * lipHeight * u3 / (radius * lipWidth);
        curvature += 12.0f * lipHeight * u2 /
            (radius * radius * lipWidth * lipWidth);
    }
}

void addContactPoint(ContactManifold& manifold, const ContactPoint& point)
{
    for (std::size_t i = 0; i < manifold.pointCount; ++i)
    {
        if (glm::length2(manifold.points[i].point - point.point) < 1e-8f)
        {
            if (point.penetration > manifold.points[i].penetration)
                manifold.points[i] = point;
            return;
        }
    }

    if (manifold.pointCount < manifold.points.size())
        manifold.points[manifold.pointCount++] = point;
}

void clipPolygonInPlace(
    std::vector<glm::vec3>& polygon,
    std::vector<glm::vec3>& scratch,
    const glm::vec3& planeNormal,
    float planeOffset)
{
    scratch.clear();
    if (polygon.empty())
        return;

    glm::vec3 previous = polygon.back();
    float previousDistance = glm::dot(planeNormal, previous) - planeOffset;
    bool previousInside = previousDistance <= kContactTolerance;

    for (const glm::vec3& current : polygon)
    {
        const float currentDistance = glm::dot(planeNormal, current) - planeOffset;
        const bool currentInside = currentDistance <= kContactTolerance;

        if (currentInside != previousInside)
        {
            const float denominator = previousDistance - currentDistance;
            if (std::fabs(denominator) > 1e-8f)
            {
                const float t = std::clamp(previousDistance / denominator, 0.0f, 1.0f);
                scratch.push_back(previous + (current - previous) * t);
            }
        }

        if (currentInside)
            scratch.push_back(current);

        previous = current;
        previousDistance = currentDistance;
        previousInside = currentInside;
    }

    polygon.swap(scratch);
    scratch.clear();
}

std::vector<glm::vec3> clipPolygon(
    const std::vector<glm::vec3>& polygon,
    const glm::vec3& planeNormal,
    float planeOffset)
{
    std::vector<glm::vec3> result;
    if (polygon.empty())
        return result;

    result.reserve(polygon.size() + 2);
    glm::vec3 previous = polygon.back();
    float previousDistance = glm::dot(planeNormal, previous) - planeOffset;
    bool previousInside = previousDistance <= kContactTolerance;

    for (const glm::vec3& current : polygon)
    {
        const float currentDistance = glm::dot(planeNormal, current) - planeOffset;
        const bool currentInside = currentDistance <= kContactTolerance;

        if (currentInside != previousInside)
        {
            const float denominator = previousDistance - currentDistance;
            if (std::fabs(denominator) > 1e-8f)
            {
                const float t = std::clamp(previousDistance / denominator, 0.0f, 1.0f);
                result.push_back(previous + (current - previous) * t);
            }
        }

        if (currentInside)
            result.push_back(current);

        previous = current;
        previousDistance = currentDistance;
        previousInside = currentInside;
    }

    return result;
}

MeshContactPositionKey contactPositionKey(const glm::vec3& point)
{
    constexpr double precision = 10000.0; // 0,1 mm
    return {
        static_cast<std::int64_t>(std::llround(point.x * precision)),
        static_cast<std::int64_t>(std::llround(point.y * precision)),
        static_cast<std::int64_t>(std::llround(point.z * precision))};
}

void reduceContactPoints(
    ContactManifold& manifold,
    std::vector<ContactPoint>& candidates,
    MeshCollisionScratch* scratch = nullptr)
{
    if (candidates.empty())
        return;

    // Malhas densas podem produzir muitos candidatos repetidos nas arestas
    // compartilhadas dos triângulos. A tabela espacial evita deduplicação O(n²).
    std::vector<ContactPoint> localUnique;
    std::vector<std::size_t> localSelected;
    std::unordered_map<MeshContactPositionKey, std::size_t,
                       MeshContactPositionKeyHash> localIndices;
    std::vector<ContactPoint>& unique = scratch
        ? scratch->uniqueCandidates : localUnique;
    std::vector<std::size_t>& selected = scratch
        ? scratch->selectedCandidates : localSelected;
    auto& indices = scratch ? scratch->contactIndices : localIndices;
    unique.clear();
    selected.clear();
    indices.clear();
    unique.reserve(candidates.size());
    indices.reserve(candidates.size());
    for (const ContactPoint& candidate : candidates)
    {
        if (!std::isfinite(candidate.point.x) || !std::isfinite(candidate.point.y) ||
            !std::isfinite(candidate.point.z))
        {
            continue;
        }

        const MeshContactPositionKey key = contactPositionKey(candidate.point);
        const auto found = indices.find(key);
        if (found != indices.end())
        {
            ContactPoint& existing = unique[found->second];
            if (candidate.penetration > existing.penetration)
                existing = candidate;
            continue;
        }

        indices.emplace(key, unique.size());
        unique.push_back(candidate);
    }

    if (unique.size() <= manifold.points.size())
    {
        for (const ContactPoint& point : unique)
            addContactPoint(manifold, point);
        return;
    }

    // Escolhe primeiro o ponto mais profundo e depois maximiza a cobertura espacial
    // do patch, evitando concentrar quatro pontos próximos em uma mesma região.
    selected.reserve(manifold.points.size());
    const auto deepest = std::max_element(
        unique.begin(), unique.end(),
        [](const ContactPoint& a, const ContactPoint& b)
        { return a.penetration < b.penetration; });
    selected.push_back(static_cast<std::size_t>(deepest - unique.begin()));

    while (selected.size() < manifold.points.size())
    {
        float bestDistance = -1.0f;
        std::size_t bestIndex = 0;
        for (std::size_t i = 0; i < unique.size(); ++i)
        {
            if (std::find(selected.begin(), selected.end(), i) != selected.end())
                continue;

            float minimumDistance = std::numeric_limits<float>::max();
            for (std::size_t chosen : selected)
            {
                minimumDistance = std::min(
                    minimumDistance,
                    glm::length2(unique[i].point - unique[chosen].point));
            }
            if (minimumDistance > bestDistance)
            {
                bestDistance = minimumDistance;
                bestIndex = i;
            }
        }
        selected.push_back(bestIndex);
    }

    for (std::size_t index : selected)
        addContactPoint(manifold, unique[index]);
}

void closestPointsOnSegments(
    const glm::vec3& p1,
    const glm::vec3& q1,
    const glm::vec3& p2,
    const glm::vec3& q2,
    glm::vec3& c1,
    glm::vec3& c2)
{
    const glm::vec3 d1 = q1 - p1;
    const glm::vec3 d2 = q2 - p2;
    const glm::vec3 r = p1 - p2;
    const float a = glm::dot(d1, d1);
    const float e = glm::dot(d2, d2);
    const float f = glm::dot(d2, r);
    float s = 0.0f;
    float t = 0.0f;

    if (a <= 1e-8f && e <= 1e-8f)
    {
        c1 = p1;
        c2 = p2;
        return;
    }

    if (a <= 1e-8f)
    {
        t = std::clamp(f / e, 0.0f, 1.0f);
    }
    else
    {
        const float c = glm::dot(d1, r);
        if (e <= 1e-8f)
        {
            s = std::clamp(-c / a, 0.0f, 1.0f);
        }
        else
        {
            const float b = glm::dot(d1, d2);
            const float denominator = a * e - b * b;
            if (std::fabs(denominator) > 1e-8f)
                s = std::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);

            t = (b * s + f) / e;
            if (t < 0.0f)
            {
                t = 0.0f;
                s = std::clamp(-c / a, 0.0f, 1.0f);
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = std::clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }

    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
}

bool buildFaceContactManifold(
    const OrientedBox& a,
    const OrientedBox& b,
    bool referenceIsA,
    int referenceAxis,
    const glm::vec3& normalAtoB,
    ContactManifold& manifold)
{
    const OrientedBox& reference = referenceIsA ? a : b;
    const OrientedBox& incident = referenceIsA ? b : a;
    const glm::vec3 referenceNormal = referenceIsA ? normalAtoB : -normalAtoB;

    const float referenceSign =
        glm::dot(reference.axes[referenceAxis], referenceNormal) >= 0.0f
            ? 1.0f : -1.0f;
    const glm::vec3 faceCenter = reference.center +
        reference.axes[referenceAxis] *
        (reference.halfExtents[referenceAxis] * referenceSign);

    int referenceSideAxes[2]{};
    int sideCount = 0;
    for (int axis = 0; axis < 3; ++axis)
        if (axis != referenceAxis)
            referenceSideAxes[sideCount++] = axis;

    const glm::vec3 referenceU = reference.axes[referenceSideAxes[0]];
    const glm::vec3 referenceV = reference.axes[referenceSideAxes[1]];
    const float referenceHalfU = reference.halfExtents[referenceSideAxes[0]];
    const float referenceHalfV = reference.halfExtents[referenceSideAxes[1]];

    int incidentAxis = 0;
    float mostAligned = -1.0f;
    for (int axis = 0; axis < 3; ++axis)
    {
        const float alignment = std::fabs(glm::dot(incident.axes[axis], referenceNormal));
        if (alignment > mostAligned)
        {
            mostAligned = alignment;
            incidentAxis = axis;
        }
    }

    const float incidentSign =
        glm::dot(incident.axes[incidentAxis], referenceNormal) > 0.0f
            ? -1.0f : 1.0f;
    const glm::vec3 incidentFaceCenter = incident.center +
        incident.axes[incidentAxis] *
        (incident.halfExtents[incidentAxis] * incidentSign);

    int incidentSideAxes[2]{};
    sideCount = 0;
    for (int axis = 0; axis < 3; ++axis)
        if (axis != incidentAxis)
            incidentSideAxes[sideCount++] = axis;

    const glm::vec3 incidentU = incident.axes[incidentSideAxes[0]] *
        incident.halfExtents[incidentSideAxes[0]];
    const glm::vec3 incidentV = incident.axes[incidentSideAxes[1]] *
        incident.halfExtents[incidentSideAxes[1]];

    std::vector<glm::vec3> polygon = {
        incidentFaceCenter - incidentU - incidentV,
        incidentFaceCenter + incidentU - incidentV,
        incidentFaceCenter + incidentU + incidentV,
        incidentFaceCenter - incidentU + incidentV};

    polygon = clipPolygon(
        polygon, referenceU,
        glm::dot(referenceU, faceCenter) + referenceHalfU);
    polygon = clipPolygon(
        polygon, -referenceU,
        glm::dot(-referenceU, faceCenter) + referenceHalfU);
    polygon = clipPolygon(
        polygon, referenceV,
        glm::dot(referenceV, faceCenter) + referenceHalfV);
    polygon = clipPolygon(
        polygon, -referenceV,
        glm::dot(-referenceV, faceCenter) + referenceHalfV);

    // Também recorta o polígono pelo plano da face de referência. Apenas
    // descartar vértices que passaram do plano perde os pontos de interseção
    // entre as faces e pode reduzir um contato de área a uma aresta ou pivô.
    polygon = clipPolygon(
        polygon,
        referenceNormal,
        glm::dot(referenceNormal, faceCenter));

    std::vector<ContactPoint> candidates;
    candidates.reserve(polygon.size());
    for (const glm::vec3& vertex : polygon)
    {
        const float signedDistance = glm::dot(referenceNormal, vertex - faceCenter);
        if (signedDistance > kContactTolerance)
            continue;

        ContactPoint point;
        point.normal = normalAtoB;
        point.penetration = std::max(0.0f, -signedDistance);
        const glm::vec3 projected = vertex - referenceNormal * signedDistance;
        point.pointA = referenceIsA ? projected : vertex;
        point.pointB = referenceIsA ? vertex : projected;
        point.point = (point.pointA + point.pointB) * 0.5f;
        point.hasSeparatePoints = true;
        candidates.push_back(point);
    }

    reduceContactPoints(manifold, candidates);
    return manifold.pointCount > 0;
}

bool buildEdgeContact(
    const OrientedBox& a,
    const OrientedBox& b,
    int axisA,
    int axisB,
    const glm::vec3& normalAtoB,
    float penetration,
    ContactManifold& manifold)
{
    glm::vec3 centerA = a.center;
    glm::vec3 centerB = b.center;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (axis != axisA)
        {
            const float sign = glm::dot(a.axes[axis], normalAtoB) >= 0.0f
                ? 1.0f : -1.0f;
            centerA += a.axes[axis] * a.halfExtents[axis] * sign;
        }
        if (axis != axisB)
        {
            const float sign = glm::dot(b.axes[axis], -normalAtoB) >= 0.0f
                ? 1.0f : -1.0f;
            centerB += b.axes[axis] * b.halfExtents[axis] * sign;
        }
    }

    const glm::vec3 extentA = a.axes[axisA] * a.halfExtents[axisA];
    const glm::vec3 extentB = b.axes[axisB] * b.halfExtents[axisB];
    glm::vec3 pointA(0.0f);
    glm::vec3 pointB(0.0f);
    closestPointsOnSegments(
        centerA - extentA, centerA + extentA,
        centerB - extentB, centerB + extentB,
        pointA, pointB);

    ContactPoint point;
    point.pointA = pointA;
    point.pointB = pointB;
    point.point = (pointA + pointB) * 0.5f;
    point.hasSeparatePoints = true;
    point.normal = normalAtoB;
    point.penetration = penetration;
    manifold.points[0] = point;
    manifold.pointCount = 1;
    return true;
}
} // namespace

OrientedBox makeOrientedBox(
    const glm::vec3& center,
    const glm::quat& orientation,
    const glm::vec3& halfExtents)
{
    OrientedBox box;
    box.center = center;
    box.axes[0] = glm::normalize(orientation * glm::vec3(1.0f, 0.0f, 0.0f));
    box.axes[1] = glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f));
    box.axes[2] = glm::normalize(orientation * glm::vec3(0.0f, 0.0f, 1.0f));
    box.halfExtents = halfExtents;
    return box;
}

float projectedRadius(const OrientedBox& box, const glm::vec3& axis)
{
    return box.halfExtents.x * std::fabs(glm::dot(box.axes[0], axis)) +
        box.halfExtents.y * std::fabs(glm::dot(box.axes[1], axis)) +
        box.halfExtents.z * std::fabs(glm::dot(box.axes[2], axis));
}

glm::vec3 supportPoint(const OrientedBox& box, const glm::vec3& direction)
{
    glm::vec3 point = box.center;
    for (int axis = 0; axis < 3; ++axis)
    {
        const float projection = glm::dot(box.axes[axis], direction);
        if (std::fabs(projection) <= 1e-6f)
            continue;
        const float sign = projection > 0.0f ? 1.0f : -1.0f;
        point += box.axes[axis] * box.halfExtents[axis] * sign;
    }
    return point;
}

glm::vec3 supportPoint(
    const RigidBody& body,
    const glm::vec3& direction,
    MeshCollisionScratch* scratch)
{
    if (!body.transform)
        return glm::vec3(0.0f);

    if (body.collider.shape == ColliderShape::Sphere)
    {
        const float directionLength = glm::length(direction);
        const glm::vec3 unitDirection = directionLength > 1e-6f
            ? direction / directionLength : glm::vec3(0.0f, 1.0f, 0.0f);
        return body.transform->position() +
            unitDirection * body.collider.sphereRadius;
    }

    if (body.collider.shape == ColliderShape::TriangleMesh &&
        body.collider.triangleMesh &&
        !body.collider.triangleMesh->vertices.empty())
    {
        const glm::quat orientation = body.transform->orientation();
        const glm::vec3 scale = body.transform->scale();
        const glm::vec3 localDirection =
            scale * (glm::transpose(glm::mat3_cast(orientation)) * direction);

        const auto& mesh = *body.collider.triangleMesh;
        const auto& vertices = mesh.vertices;
        const bool collectStats = scratch && scratch->collectStats;
        const std::uint32_t vertexGeneration =
            scratch && mesh.supportVertexIndices.empty() &&
                !mesh.bvhNodes.empty()
            ? beginVertexStampGeneration(*scratch, vertices.size())
            : 0u;
        if (collectStats)
            ++scratch->counters.supportQueries;
        std::size_t bestIndex = 0;
        float bestProjection = -std::numeric_limits<float>::max();

        if (!mesh.supportVertexIndices.empty())
        {
            for (const unsigned int vertexIndex : mesh.supportVertexIndices)
            {
                if (collectStats)
                    ++scratch->counters.supportVerticesTested;
                const float projection = glm::dot(
                    vertices[vertexIndex], localDirection);
                if (projection > bestProjection)
                {
                    bestProjection = projection;
                    bestIndex = vertexIndex;
                }
            }
        }
        else if (!mesh.bvhNodes.empty())
        {
            // Busca de suporte branch-and-bound: bounds de nós que não podem
            // superar o melhor vértice conhecido são descartados sem percorrer
            // seus triângulos.
            std::array<std::uint32_t, 128> stack{};
            std::size_t stackSize = 0;
            stack[stackSize++] = 0;
            const auto maximumProjection = [&localDirection](
                const TriangleMeshBvhNode& node)
            {
                float value = 0.0f;
                for (int axis = 0; axis < 3; ++axis)
                    value += localDirection[axis] *
                        (localDirection[axis] >= 0.0f
                            ? node.boundsMax[axis] : node.boundsMin[axis]);
                return value;
            };

            while (stackSize > 0)
            {
                const std::uint32_t nodeIndex = stack[--stackSize];
                const TriangleMeshBvhNode& node = mesh.bvhNodes[nodeIndex];
                if (collectStats)
                    ++scratch->counters.supportBvhNodesVisited;
                if (maximumProjection(node) < bestProjection - 1e-7f)
                {
                    if (collectStats)
                        ++scratch->counters.supportBvhNodesRejected;
                    continue;
                }

                if (node.isLeaf())
                {
                    for (std::uint32_t i = 0; i < node.triangleCount; ++i)
                    {
                        const std::size_t base = static_cast<std::size_t>(
                            mesh.triangleOrder[node.firstTriangle + i]) * 3;
                        for (std::size_t corner = 0; corner < 3; ++corner)
                        {
                            const std::size_t vertexIndex = mesh.triangleIndices[base + corner];
                            if (scratch)
                            {
                                if (scratch->vertexStamps[vertexIndex] ==
                                    vertexGeneration)
                                    continue;
                                scratch->vertexStamps[vertexIndex] =
                                    vertexGeneration;
                            }
                            if (collectStats)
                                ++scratch->counters.supportVerticesTested;
                            const float projection = glm::dot(
                                vertices[vertexIndex], localDirection);
                            if (projection > bestProjection)
                            {
                                bestProjection = projection;
                                bestIndex = vertexIndex;
                            }
                        }
                    }
                }
                else
                {
                    const std::uint32_t left = node.leftChild;
                    const std::uint32_t right = node.rightChild;
                    const float leftBound = maximumProjection(mesh.bvhNodes[left]);
                    const float rightBound = maximumProjection(mesh.bvhNodes[right]);
                    // O maior limite é visitado primeiro para aumentar a poda.
                    const std::uint32_t first = leftBound >= rightBound ? left : right;
                    const std::uint32_t second = leftBound >= rightBound ? right : left;
                    if (stackSize + 2 <= stack.size())
                    {
                        stack[stackSize++] = second;
                        stack[stackSize++] = first;
                    }
                }
            }
        }
        else
        {
            for (std::size_t i = 0; i < vertices.size(); ++i)
            {
                if (collectStats)
                    ++scratch->counters.supportVerticesTested;
                const float projection = glm::dot(vertices[i], localDirection);
                if (projection > bestProjection)
                {
                    bestProjection = projection;
                    bestIndex = i;
                }
            }
        }

        return body.transform->position() +
            orientation * (scale * vertices[bestIndex]);
    }

    const OrientedBox box = makeOrientedBox(
        body.transform->position(), body.transform->orientation(),
        body.collider.boxHalfExtents);
    return supportPoint(box, direction);
}

float supportRadius(
    const RigidBody& body,
    const glm::vec3& direction,
    MeshCollisionScratch* scratch)
{
    if (body.collider.shape == ColliderShape::Sphere)
        return body.collider.sphereRadius;
    if (!body.transform)
        return 0.0f;

    if (body.collider.shape == ColliderShape::TriangleMesh &&
        body.collider.triangleMesh)
    {
        const float directionLength = glm::length(direction);
        if (directionLength <= 1e-6f)
            return 0.0f;
        const glm::vec3 unitDirection = direction / directionLength;
        return std::max(0.0f, glm::dot(
            supportPoint(body, unitDirection, scratch) - body.transform->position(),
            unitDirection));
    }

    const OrientedBox box = makeOrientedBox(
        body.transform->position(), body.transform->orientation(),
        body.collider.boxHalfExtents);
    return projectedRadius(box, direction);
}

bool intersectOrientedBoxes(
    const OrientedBox& a,
    const OrientedBox& b,
    ContactManifold& manifold)
{
    manifold.pointCount = 0;
    const glm::vec3 centerDelta = b.center - a.center;
    float minimumOverlap = std::numeric_limits<float>::max();
    glm::vec3 minimumAxis(0.0f);
    enum class Feature { FaceA, FaceB, EdgeEdge };
    Feature minimumFeature = Feature::FaceA;
    int minimumAxisA = -1;
    int minimumAxisB = -1;

    const auto testAxis = [&](const glm::vec3& candidate,
                              Feature feature,
                              int axisA,
                              int axisB) -> bool
    {
        const float lengthSquared = glm::dot(candidate, candidate);
        if (lengthSquared <= 1e-6f)
            return true;

        const glm::vec3 axis = candidate / std::sqrt(lengthSquared);
        const float distance = std::fabs(glm::dot(centerDelta, axis));
        const float overlap = projectedRadius(a, axis) +
            projectedRadius(b, axis) - distance;
        if (overlap < -kContactTolerance)
            return false;

        if (overlap < minimumOverlap)
        {
            minimumOverlap = overlap;
            minimumAxis = glm::dot(centerDelta, axis) >= 0.0f ? axis : -axis;
            minimumFeature = feature;
            minimumAxisA = axisA;
            minimumAxisB = axisB;
        }
        return true;
    };

    for (int i = 0; i < 3; ++i)
        if (!testAxis(a.axes[i], Feature::FaceA, i, -1))
            return false;
    for (int i = 0; i < 3; ++i)
        if (!testAxis(b.axes[i], Feature::FaceB, -1, i))
            return false;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (!testAxis(glm::cross(a.axes[i], b.axes[j]), Feature::EdgeEdge, i, j))
                return false;

    if (minimumOverlap == std::numeric_limits<float>::max())
        return false;

    bool generated = false;
    if (minimumFeature == Feature::FaceA)
    {
        generated = buildFaceContactManifold(
            a, b, true, minimumAxisA, minimumAxis, manifold);
    }
    else if (minimumFeature == Feature::FaceB)
    {
        generated = buildFaceContactManifold(
            a, b, false, minimumAxisB, minimumAxis, manifold);
    }
    else
    {
        generated = buildEdgeContact(
            a, b, minimumAxisA, minimumAxisB,
            minimumAxis, minimumOverlap, manifold);
    }

    if (!generated)
    {
        ContactPoint fallback;
        fallback.normal = minimumAxis;
        fallback.penetration = std::max(0.0f, minimumOverlap);
        const glm::vec3 pointA = supportPoint(a, minimumAxis);
        const glm::vec3 pointB = supportPoint(b, -minimumAxis);
        fallback.pointA = pointA;
        fallback.pointB = pointB;
        fallback.point = (pointA + pointB) * 0.5f;
        fallback.hasSeparatePoints = true;
        manifold.points[0] = fallback;
        manifold.pointCount = 1;
    }

    return manifold.pointCount > 0;
}

bool intersectOrientedBoxes(
    const OrientedBox& a,
    const OrientedBox& b,
    ContactPoint& contact)
{
    ContactManifold manifold;
    if (!intersectOrientedBoxes(a, b, manifold))
        return false;
    contact = manifold.points[0];
    return true;
}

bool boxPlaneContacts(
    const OrientedBox& box,
    const glm::vec3& planeNormal,
    float planeOffset,
    ContactManifold& manifold)
{
    manifold.pointCount = 0;
    const float normalLength = glm::length(planeNormal);
    if (normalLength <= 1e-6f)
        return false;
    const glm::vec3 normal = planeNormal / normalLength;
    const float normalizedOffset = planeOffset / normalLength;

    std::array<glm::vec3, 8> vertices{};
    std::array<float, 8> signedDistances{};
    std::size_t vertexCount = 0;
    float minimumDistance = std::numeric_limits<float>::max();

    for (int x = -1; x <= 1; x += 2)
    for (int y = -1; y <= 1; y += 2)
    for (int z = -1; z <= 1; z += 2)
    {
        const glm::vec3 vertex = box.center +
            box.axes[0] * (box.halfExtents.x * static_cast<float>(x)) +
            box.axes[1] * (box.halfExtents.y * static_cast<float>(y)) +
            box.axes[2] * (box.halfExtents.z * static_cast<float>(z));
        const float signedDistance = glm::dot(normal, vertex) - normalizedOffset;
        vertices[vertexCount] = vertex;
        signedDistances[vertexCount] = signedDistance;
        minimumDistance = std::min(minimumDistance, signedDistance);
        ++vertexCount;
    }

    if (minimumDistance > kContactTolerance)
        return false;

    // Usa somente os vértices do feature de suporte mais baixo. Quando uma
    // caixa atravessa profundamente o plano, incluir os oito vértices criaria
    // contatos nas laterais e poderia aplicar torque artificial.
    for (std::size_t i = 0; i < vertexCount; ++i)
    {
        const float signedDistance = signedDistances[i];
        if (signedDistance > minimumDistance + kContactTolerance)
            continue;

        const glm::vec3& vertex = vertices[i];
        ContactPoint point;
        point.normal = normal;
        point.penetration = std::max(0.0f, -signedDistance);
        point.pointA = vertex - normal * signedDistance;
        point.pointB = vertex;
        point.point = (point.pointA + point.pointB) * 0.5f;
        point.hasSeparatePoints = true;
        addContactPoint(manifold, point);
    }

    return manifold.pointCount > 0;
}

namespace
{
MeshCollisionScratch& resolveScratch(
    MeshCollisionScratch* supplied,
    MeshCollisionScratch& local)
{
    return supplied ? *supplied : local;
}

std::uint32_t beginVertexGeneration(MeshCollisionScratch& scratch, std::size_t vertexCount)
{
    const std::uint32_t generation =
        beginVertexStampGeneration(scratch, vertexCount);
    if (scratch.transformedVertices.size() < vertexCount)
        scratch.transformedVertices.resize(vertexCount);

    return generation;
}

std::uint32_t beginVertexStampGeneration(
    MeshCollisionScratch& scratch,
    std::size_t vertexCount)
{
    if (scratch.vertexStamps.size() < vertexCount)
        scratch.vertexStamps.resize(vertexCount, 0u);

    ++scratch.generation;
    if (scratch.generation == 0u)
    {
        std::fill(scratch.vertexStamps.begin(), scratch.vertexStamps.end(), 0u);
        scratch.generation = 1u;
    }
    return scratch.generation;
}

bool overlapsAabb(
    const glm::vec3& aMin, const glm::vec3& aMax,
    const TriangleMeshBvhNode& node)
{
    return !(aMax.x < node.boundsMin.x || aMin.x > node.boundsMax.x ||
             aMax.y < node.boundsMin.y || aMin.y > node.boundsMax.y ||
             aMax.z < node.boundsMin.z || aMin.z > node.boundsMax.z);
}

void queryMeshBvhAabb(
    const TriangleMeshColliderData& mesh,
    const glm::vec3& queryMin,
    const glm::vec3& queryMax,
    MeshCollisionScratch& scratch)
{
    scratch.candidateTriangles.clear();
    scratch.traversalStack.clear();
    if (mesh.bvhNodes.empty())
    {
        const std::size_t count = mesh.triangleIndices.size() / 3;
        scratch.candidateTriangles.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            scratch.candidateTriangles.push_back(i);
            if (scratch.collectStats)
                ++scratch.counters.candidateTriangles;
        }
        return;
    }

    scratch.traversalStack.push_back(0u);
    while (!scratch.traversalStack.empty())
    {
        const std::uint32_t index = scratch.traversalStack.back();
        scratch.traversalStack.pop_back();
        const TriangleMeshBvhNode& node = mesh.bvhNodes[index];
        if (scratch.collectStats)
            ++scratch.counters.bvhNodesVisited;
        if (!overlapsAabb(queryMin, queryMax, node))
        {
            if (scratch.collectStats)
                ++scratch.counters.bvhNodesRejected;
            continue;
        }

        if (node.isLeaf())
        {
            for (std::uint32_t i = 0; i < node.triangleCount; ++i)
            {
                scratch.candidateTriangles.push_back(
                    mesh.triangleOrder[node.firstTriangle + i]);
                if (scratch.collectStats)
                    ++scratch.counters.candidateTriangles;
            }
        }
        else
        {
            scratch.traversalStack.push_back(node.leftChild);
            scratch.traversalStack.push_back(node.rightChild);
        }
    }
}

float minimumNodeProjection(
    const TriangleMeshBvhNode& node,
    const glm::vec3& direction)
{
    float value = 0.0f;
    for (int axis = 0; axis < 3; ++axis)
        value += direction[axis] *
            (direction[axis] >= 0.0f
                ? node.boundsMin[axis] : node.boundsMax[axis]);
    return value;
}
} // namespace

bool triangleMeshPlaneContacts(
    const RigidBody& body,
    const glm::vec3& planeNormal,
    float planeOffset,
    ContactManifold& manifold,
    MeshCollisionScratch* suppliedScratch)
{
    manifold.clear();
    if (!body.transform || !body.collider.triangleMesh ||
        body.collider.triangleMesh->vertices.empty())
        return false;

    const float normalLength = glm::length(planeNormal);
    if (normalLength <= 1e-6f || !std::isfinite(normalLength))
        return false;

    const glm::vec3 normal = planeNormal / normalLength;
    const float normalizedOffset = planeOffset / normalLength;
    const glm::quat orientation = body.transform->orientation();
    const glm::vec3 scale = body.transform->scale();
    const glm::vec3 bodyPosition = body.transform->position();
    constexpr float contactTolerance = 0.0001f;

    MeshCollisionScratch localScratch;
    MeshCollisionScratch& scratch = resolveScratch(suppliedScratch, localScratch);
    if (scratch.collectStats)
        ++scratch.counters.contactQueries;
    const glm::vec3 lowestPoint = supportPoint(body, -normal, &scratch);
    const float minimumDistance =
        glm::dot(normal, lowestPoint) - normalizedOffset;
    if (minimumDistance > contactTolerance)
        return false;

    const auto& mesh = *body.collider.triangleMesh;
    scratch.candidates.clear();
    scratch.candidates.reserve(std::min<std::size_t>(
        mesh.triangleIndices.size() / 3, 256));
    const std::uint32_t generation = beginVertexGeneration(scratch, mesh.vertices.size());

    // O plano vira um funcional linear no espaço local. Os bounds da BVH
    // permitem descartar nós cuja menor projeção já está acima do suporte.
    const glm::vec3 localPlaneDirection = scale *
        (glm::transpose(glm::mat3_cast(orientation)) * normal);
    const float localPlaneOffset = glm::dot(normal, bodyPosition) - normalizedOffset;
    const float maximumContactDistance = minimumDistance + contactTolerance;

    const auto appendVertex = [&](std::size_t vertexIndex)
    {
        if (vertexIndex >= mesh.vertices.size() ||
            scratch.vertexStamps[vertexIndex] == generation)
            return;
        scratch.vertexStamps[vertexIndex] = generation;
        if (scratch.collectStats)
            ++scratch.counters.verticesTested;

        const glm::vec3& localVertex = mesh.vertices[vertexIndex];
        const float signedDistance = localPlaneOffset +
            glm::dot(localPlaneDirection, localVertex);
        if (signedDistance > maximumContactDistance)
            return;

        const glm::vec3 worldVertex =
            bodyPosition + orientation * (scale * localVertex);
        ContactPoint point;
        point.normal = normal;
        point.penetration = std::max(0.0f, -signedDistance);
        point.pointB = worldVertex;
        point.pointA = worldVertex - normal * signedDistance;
        point.point = (point.pointA + point.pointB) * 0.5f;
        point.hasSeparatePoints = true;
        scratch.candidates.push_back(point);
    };

    if (!mesh.bvhNodes.empty())
    {
        scratch.traversalStack.clear();
        scratch.traversalStack.push_back(0u);
        while (!scratch.traversalStack.empty())
        {
            const std::uint32_t nodeIndex = scratch.traversalStack.back();
            scratch.traversalStack.pop_back();
            const TriangleMeshBvhNode& node = mesh.bvhNodes[nodeIndex];
            if (scratch.collectStats)
                ++scratch.counters.bvhNodesVisited;
            const float nodeMinimum = localPlaneOffset +
                minimumNodeProjection(node, localPlaneDirection);
            if (nodeMinimum > maximumContactDistance)
            {
                if (scratch.collectStats)
                    ++scratch.counters.bvhNodesRejected;
                continue;
            }

            if (node.isLeaf())
            {
                for (std::uint32_t i = 0; i < node.triangleCount; ++i)
                {
                    if (scratch.collectStats)
                        ++scratch.counters.candidateTriangles;
                    const std::size_t base = static_cast<std::size_t>(
                        mesh.triangleOrder[node.firstTriangle + i]) * 3;
                    appendVertex(mesh.triangleIndices[base]);
                    appendVertex(mesh.triangleIndices[base + 1]);
                    appendVertex(mesh.triangleIndices[base + 2]);
                }
            }
            else
            {
                scratch.traversalStack.push_back(node.leftChild);
                scratch.traversalStack.push_back(node.rightChild);
            }
        }
    }
    else
    {
        for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
            appendVertex(i);
    }

    reduceContactPoints(manifold, scratch.candidates, &scratch);
    return !manifold.empty();
}

bool triangleMeshOrientedBoxContacts(
    const RigidBody& body,
    const OrientedBox& box,
    ContactManifold& manifold,
    MeshCollisionScratch* suppliedScratch)
{
    manifold.clear();
    if (!body.transform || !body.collider.triangleMesh ||
        body.collider.triangleMesh->vertices.empty() ||
        body.collider.triangleMesh->triangleIndices.size() < 3)
        return false;

    const glm::quat orientation = body.transform->orientation();
    const glm::mat3 rotation = glm::mat3_cast(orientation);
    const glm::mat3 inverseRotation = glm::transpose(rotation);
    const glm::vec3 scale = body.transform->scale();
    if (std::fabs(scale.x) <= 1e-8f || std::fabs(scale.y) <= 1e-8f ||
        std::fabs(scale.z) <= 1e-8f)
        return false;

    const glm::vec3 bodyPosition = body.transform->position();
    MeshCollisionScratch localScratch;
    MeshCollisionScratch& scratch = resolveScratch(suppliedScratch, localScratch);
    if (scratch.collectStats)
        ++scratch.counters.contactQueries;
    const glm::mat3 boxBasis(box.axes[0], box.axes[1], box.axes[2]);
    const glm::mat3 inverseBasis = glm::transpose(boxBasis);

    const OrientedBox meshBounds = makeOrientedBox(
        bodyPosition + orientation * body.collider.boxCenter,
        orientation,
        body.collider.boxHalfExtents);
    ContactManifold broadphase;
    if (!intersectOrientedBoxes(meshBounds, box, broadphase))
        return false;

    const auto& mesh = *body.collider.triangleMesh;

    // Quando o eixo mínimo do SAT coincide com uma face da caixa estática,
    // basta consultar o feature de suporte da malha contra o plano dessa face.
    // Isso evita percorrer todos os triângulos que ficam dentro do volume grande
    // de uma mesa/plataforma, mantendo a consulta por triângulos como fallback
    // para contatos oblíquos, de aresta ou geometrias não cobertas pelo feature.
    if (!broadphase.empty())
    {
        const glm::vec3 boxToMeshNormal =
            -glm::normalize(broadphase.points[0].normal);
        int alignedFaceAxis = -1;
        float faceAlignment = 0.9995f;
        for (int axis = 0; axis < 3; ++axis)
        {
            const float alignment = std::fabs(
                glm::dot(box.axes[axis], boxToMeshNormal));
            if (alignment > faceAlignment)
            {
                faceAlignment = alignment;
                alignedFaceAxis = axis;
            }
        }

        if (alignedFaceAxis >= 0)
        {
            const float side = glm::dot(
                box.axes[alignedFaceAxis], boxToMeshNormal) >= 0.0f
                    ? 1.0f : -1.0f;
            const glm::vec3 faceNormal = box.axes[alignedFaceAxis] * side;
            const float faceOffset = glm::dot(faceNormal, box.center) +
                box.halfExtents[alignedFaceAxis];
            ContactManifold supportManifold;
            const std::uint64_t queriesBeforeSupport = scratch.collectStats
                ? scratch.counters.contactQueries : 0;
            const bool hasSupportContacts = triangleMeshPlaneContacts(
                body, faceNormal, faceOffset, supportManifold, suppliedScratch);
            // O relatório conta a consulta externa de mesh-vs-box uma vez; a
            // chamada interna ao helper de plano é parte dessa mesma consulta.
            if (scratch.collectStats)
                scratch.counters.contactQueries = queriesBeforeSupport;
            if (hasSupportContacts)
            {
                bool keptAnyPoint = false;
                for (std::size_t pointIndex = 0;
                     pointIndex < supportManifold.pointCount; ++pointIndex)
                {
                    const ContactPoint& point = supportManifold.points[pointIndex];
                    const glm::vec3 localPoint =
                        inverseBasis * (point.pointB - box.center);
                    bool insideFace = true;
                    for (int axis = 0; axis < 3; ++axis)
                    {
                        if (axis == alignedFaceAxis)
                            continue;
                        if (std::fabs(localPoint[axis]) >
                            box.halfExtents[axis] + 0.0001f)
                        {
                            insideFace = false;
                            break;
                        }
                    }
                    if (insideFace)
                    {
                        addContactPoint(manifold, point);
                        keptAnyPoint = true;
                    }
                }
                if (keptAnyPoint)
                    return true;
                manifold.clear();
            }
        }
    }

    // Encapsula a OBB consultada no espaço local da malha. A AABB resultante
    // é conservadora mesmo com rotação e escala não uniforme.
    glm::vec3 queryMin(std::numeric_limits<float>::max());
    glm::vec3 queryMax(std::numeric_limits<float>::lowest());
    for (int x = -1; x <= 1; x += 2)
    for (int y = -1; y <= 1; y += 2)
    for (int z = -1; z <= 1; z += 2)
    {
        const glm::vec3 worldCorner = box.center +
            box.axes[0] * (box.halfExtents.x * static_cast<float>(x)) +
            box.axes[1] * (box.halfExtents.y * static_cast<float>(y)) +
            box.axes[2] * (box.halfExtents.z * static_cast<float>(z));
        const glm::vec3 rotated = inverseRotation * (worldCorner - bodyPosition);
        const glm::vec3 localCorner(
            rotated.x / scale.x, rotated.y / scale.y, rotated.z / scale.z);
        queryMin = glm::min(queryMin, localCorner);
        queryMax = glm::max(queryMax, localCorner);
    }
    const glm::vec3 localTolerance(
        0.0001f / std::fabs(scale.x),
        0.0001f / std::fabs(scale.y),
        0.0001f / std::fabs(scale.z));
    queryMeshBvhAabb(
        mesh, queryMin - localTolerance, queryMax + localTolerance, scratch);
    if (scratch.candidateTriangles.empty())
        return false;

    const std::uint32_t generation = beginVertexGeneration(scratch, mesh.vertices.size());
    const auto toBoxLocal = [&](std::size_t vertexIndex) -> const glm::vec3&
    {
        if (scratch.vertexStamps[vertexIndex] != generation)
        {
            const glm::vec3 worldVertex = bodyPosition +
                orientation * (scale * mesh.vertices[vertexIndex]);
            scratch.transformedVertices[vertexIndex] =
                inverseBasis * (worldVertex - box.center);
            scratch.vertexStamps[vertexIndex] = generation;
        }
        return scratch.transformedVertices[vertexIndex];
    };

    scratch.candidates.clear();
    scratch.candidates.reserve(std::min<std::size_t>(
        scratch.candidateTriangles.size() * 3, 512));
    scratch.polygon.clear();
    scratch.polygonScratch.clear();
    scratch.polygon.reserve(12);
    scratch.polygonScratch.reserve(12);
    constexpr float contactTolerance = 0.0001f;

    for (const std::uint32_t triangle : scratch.candidateTriangles)
    {
        if (scratch.collectStats)
            ++scratch.counters.trianglesTested;
        const std::size_t base = static_cast<std::size_t>(triangle) * 3;
        const unsigned int ia = mesh.triangleIndices[base];
        const unsigned int ib = mesh.triangleIndices[base + 1];
        const unsigned int ic = mesh.triangleIndices[base + 2];
        if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() ||
            ic >= mesh.vertices.size())
            continue;

        const glm::vec3& a = toBoxLocal(ia);
        const glm::vec3& b = toBoxLocal(ib);
        const glm::vec3& c = toBoxLocal(ic);
        const glm::vec3 triangleMin = glm::min(a, glm::min(b, c));
        const glm::vec3 triangleMax = glm::max(a, glm::max(b, c));
        if (triangleMax.x < -box.halfExtents.x - contactTolerance ||
            triangleMin.x > box.halfExtents.x + contactTolerance ||
            triangleMax.y < -box.halfExtents.y - contactTolerance ||
            triangleMin.y > box.halfExtents.y + contactTolerance ||
            triangleMax.z < -box.halfExtents.z - contactTolerance ||
            triangleMin.z > box.halfExtents.z + contactTolerance)
            continue;

        scratch.polygon.clear();
        scratch.polygon.push_back(a);
        scratch.polygon.push_back(b);
        scratch.polygon.push_back(c);

        for (int axis = 0; axis < 3 && !scratch.polygon.empty(); ++axis)
        {
            glm::vec3 normal(0.0f);
            normal[axis] = 1.0f;
            clipPolygonInPlace(
                scratch.polygon, scratch.polygonScratch, normal,
                box.halfExtents[axis] + contactTolerance);
            normal[axis] = -1.0f;
            clipPolygonInPlace(
                scratch.polygon, scratch.polygonScratch, normal,
                box.halfExtents[axis] + contactTolerance);
        }
        if (scratch.polygon.empty())
            continue;

        for (const glm::vec3& localPoint : scratch.polygon)
        {
            int closestAxis = 0;
            float closestDistance = box.halfExtents.x - std::fabs(localPoint.x);
            for (int axis = 1; axis < 3; ++axis)
            {
                const float distance =
                    box.halfExtents[axis] - std::fabs(localPoint[axis]);
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestAxis = axis;
                }
            }

            const float side = localPoint[closestAxis] >= 0.0f ? 1.0f : -1.0f;
            const glm::vec3 normal = box.axes[closestAxis] * side;
            glm::vec3 boxSurfaceLocal = localPoint;
            boxSurfaceLocal[closestAxis] =
                side * box.halfExtents[closestAxis];

            const glm::vec3 meshPoint = box.center + boxBasis * localPoint;
            const glm::vec3 boxSurfacePoint =
                box.center + boxBasis * boxSurfaceLocal;

            ContactPoint point;
            point.normal = normal;
            point.penetration = std::max(0.0f, closestDistance);
            point.pointA = boxSurfacePoint;
            point.pointB = meshPoint;
            point.point = (point.pointA + point.pointB) * 0.5f;
            point.hasSeparatePoints = true;
            scratch.candidates.push_back(point);
        }
    }

    reduceContactPoints(manifold, scratch.candidates, &scratch);
    return !manifold.empty();
}

bool sphereOrientedBoxContact(
    const glm::vec3& sphereCenter,
    float sphereRadius,
    const OrientedBox& box,
    ContactPoint& contact)
{
    const glm::vec3 offset = sphereCenter - box.center;
    const glm::vec3 localCenter(
        glm::dot(offset, box.axes[0]),
        glm::dot(offset, box.axes[1]),
        glm::dot(offset, box.axes[2]));
    glm::vec3 closestLocal = glm::clamp(
        localCenter, -box.halfExtents, box.halfExtents);
    const glm::vec3 localDelta = localCenter - closestLocal;
    const float distanceSquared = glm::dot(localDelta, localDelta);

    if (distanceSquared > sphereRadius * sphereRadius)
        return false;

    glm::vec3 normalLocal(0.0f);
    if (distanceSquared > 1e-12f)
    {
        const float distance = std::sqrt(distanceSquared);
        normalLocal = localDelta / distance;
        contact.penetration = sphereRadius - distance;
    }
    else
    {
        const glm::vec3 faceDistances =
            box.halfExtents - glm::abs(localCenter);
        int axis = 0;
        if (faceDistances.y < faceDistances.x)
            axis = 1;
        if (faceDistances.z < faceDistances[axis])
            axis = 2;

        const float sign = localCenter[axis] >= 0.0f ? 1.0f : -1.0f;
        normalLocal[axis] = sign;
        closestLocal = localCenter;
        closestLocal[axis] = sign * box.halfExtents[axis];
        contact.penetration = sphereRadius + std::max(faceDistances[axis], 0.0f);
    }

    contact.normal = glm::normalize(
        box.axes[0] * normalLocal.x +
        box.axes[1] * normalLocal.y +
        box.axes[2] * normalLocal.z);
    contact.pointA = box.center +
        box.axes[0] * closestLocal.x +
        box.axes[1] * closestLocal.y +
        box.axes[2] * closestLocal.z;
    contact.pointB = sphereCenter - contact.normal * sphereRadius;
    contact.point = (contact.pointA + contact.pointB) * 0.5f;
    contact.hasSeparatePoints = true;
    return true;
}

bool sphereSphereContact(
    const glm::vec3& centerA,
    float radiusA,
    const glm::vec3& centerB,
    float radiusB,
    ContactPoint& contact)
{
    const glm::vec3 delta = centerB - centerA;
    const float distanceSquared = glm::dot(delta, delta);
    const float combinedRadius = radiusA + radiusB;
    if (distanceSquared >= combinedRadius * combinedRadius)
        return false;

    const float distance = std::sqrt(std::max(0.0f, distanceSquared));
    contact.normal = distance > 1e-6f
        ? delta / distance : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 pointA = centerA + contact.normal * radiusA;
    const glm::vec3 pointB = centerB - contact.normal * radiusB;
    contact.pointA = pointA;
    contact.pointB = pointB;
    contact.point = (pointA + pointB) * 0.5f;
    contact.hasSeparatePoints = true;
    contact.penetration = combinedRadius - distance;
    return true;
}

BowlSurfaceSample findClosestBowlSurface(
    const glm::vec3& position,
    float radius,
    float depth,
    float lipHeight)
{
    const float radialDistance = std::sqrt(
        position.x * position.x + position.z * position.z);
    const float directionX = radialDistance > 1e-6f
        ? position.x / radialDistance : 1.0f;
    const float directionZ = radialDistance > 1e-6f
        ? position.z / radialDistance : 0.0f;

    float surfaceRadius = std::clamp(radialDistance, 0.0f, radius);
    for (int iteration = 0; iteration < 10; ++iteration)
    {
        float height = 0.0f;
        float slope = 0.0f;
        float curvature = 0.0f;
        evaluateBowlProfile(
            radius, depth, lipHeight, surfaceRadius,
            height, slope, curvature);

        const float function = (surfaceRadius - radialDistance) +
            (height - position.y) * slope;
        const float derivative = 1.0f + slope * slope +
            (height - position.y) * curvature;
        if (std::fabs(derivative) <= 1e-6f)
            break;

        const float nextRadius = std::clamp(
            surfaceRadius - function / derivative, 0.0f, radius);
        if (std::fabs(nextRadius - surfaceRadius) <= 1e-5f)
        {
            surfaceRadius = nextRadius;
            break;
        }
        surfaceRadius = nextRadius;
    }

    float surfaceHeight = 0.0f;
    float surfaceSlope = 0.0f;
    float ignoredCurvature = 0.0f;
    evaluateBowlProfile(
        radius, depth, lipHeight, surfaceRadius,
        surfaceHeight, surfaceSlope, ignoredCurvature);

    BowlSurfaceSample sample;
    sample.surfacePoint = glm::vec3(
        directionX * surfaceRadius,
        surfaceHeight,
        directionZ * surfaceRadius);
    sample.normal = glm::normalize(glm::vec3(
        -surfaceSlope * directionX,
        1.0f,
        -surfaceSlope * directionZ));
    sample.signedDistance = glm::dot(
        position - sample.surfacePoint, sample.normal);
    return sample;
}
} // namespace rotor::physics::collision
