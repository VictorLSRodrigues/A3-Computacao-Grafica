#include "engine/physics/collision/collision_geometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <glm/gtx/norm.hpp>
#include <vector>

namespace rotor::physics::collision
{
namespace
{
constexpr float kBowlLipStart = 0.78f;
constexpr float kContactTolerance = 1e-4f;

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

void reduceContactPoints(ContactManifold& manifold, std::vector<ContactPoint>& candidates)
{
    if (candidates.empty())
        return;

    // Elimina pontos praticamente coincidentes.
    std::vector<ContactPoint> unique;
    unique.reserve(candidates.size());
    for (const ContactPoint& candidate : candidates)
    {
        bool duplicate = false;
        for (ContactPoint& existing : unique)
        {
            if (glm::length2(existing.point - candidate.point) < 1e-8f)
            {
                existing.penetration = std::max(existing.penetration, candidate.penetration);
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
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
    std::vector<std::size_t> selected;
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

glm::vec3 supportPoint(const RigidBody& body, const glm::vec3& direction)
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

    const OrientedBox box = makeOrientedBox(
        body.transform->position(), body.transform->orientation(),
        body.collider.boxHalfExtents);
    return supportPoint(box, direction);
}

float supportRadius(const RigidBody& body, const glm::vec3& direction)
{
    if (body.collider.shape == ColliderShape::Sphere)
        return body.collider.sphereRadius;
    if (!body.transform)
        return 0.0f;

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
