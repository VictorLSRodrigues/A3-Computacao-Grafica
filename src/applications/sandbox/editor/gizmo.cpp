#include "applications/sandbox/editor/gizmo.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/scene/picking.h"

bool buildExtrudeGizmoProjection(
    const Window &window,
    const Camera &camera,
    float fovDegrees,
    const Scene &scene,
    ExtrudeGizmoProjection &result)
{
    if (!scene.getSelectedFace().valid)
        return false;

    glm::vec3 a, b, c;

    if (!scene.getSelectedFacePositions(a, b, c))
        return false;

    // A seta começa no primeiro vértice da face.
    result.worldCenter = a;

    // Calcula a normal da face.
    glm::vec3 normal = glm::cross(b - a, c - a);

    if (glm::length(normal) < 0.00001f)
        return false;

    result.worldNormal = glm::normalize(normal);

    // Dimensões da janela.
    const int width = window.getWidth();
    const int height = window.getHeight();

    if (width <= 0 || height <= 0)
        return false;

    // Verifica a distância da face em relação à câmera.
    const glm::vec3 forward =
        glm::normalize(camera.orientation);

    const float depth = glm::dot(
        result.worldCenter - camera.position,
        forward);

    if (depth <= 0.05f)
        return false;

    // Matrizes da câmera.
    const glm::mat4 viewProjection = camera.viewProjectionMatrix(fovDegrees);

    // Projeta os três vértices da face para a tela.
    if (!projectWorldToScreen(
            a,
            viewProjection,
            width,
            height,
            result.screenStart))
        return false;

    if (!projectWorldToScreen(
            b,
            viewProjection,
            width,
            height,
            result.screenFaceB))
        return false;

    if (!projectWorldToScreen(
            c,
            viewProjection,
            width,
            height,
            result.screenFaceC))
        return false;

    // Mantém o tamanho visual da seta.
    result.worldLength = std::clamp(
        2.0f * depth *
            std::tan(glm::radians(fovDegrees) * 0.5f) *
            85.0f / static_cast<float>(height),
        0.05f,
        5.0f);

    const glm::vec3 endPosition =
        result.worldCenter +
        result.worldNormal * result.worldLength;

    if (!projectWorldToScreen(
            endPosition,
            viewProjection,
            width,
            height,
            result.screenEnd))
        return false;

    // Calcula a direção da seta na tela.
    const glm::vec2 screenDelta =
        result.screenEnd - result.screenStart;

    const float screenLength = glm::length(screenDelta);

    if (screenLength < 5.0f)
        return false;

    result.screenDirection = screenDelta / screenLength;

    result.worldPerPixel =
        result.worldLength / screenLength;

    result.visible = true;

    return true;
}

namespace
{
    float distanceToExtrudeGizmo(
        const glm::vec2 &mouse,
        const ExtrudeGizmoProjection &gizmo)
    {
        const glm::vec2 segment =
            gizmo.screenEnd - gizmo.screenStart;

        const float lengthSquared =
            glm::dot(segment, segment);

        if (lengthSquared <= 0.0001f)
            return std::numeric_limits<float>::max();

        const float t = std::clamp(
            glm::dot(mouse - gizmo.screenStart, segment) /
                lengthSquared,
            0.0f,
            1.0f);

        const glm::vec2 closestPoint =
            gizmo.screenStart + segment * t;

        return glm::length(mouse - closestPoint);
    }

    bool isPointInsideTriangle(
        const glm::vec2 &p,
        const glm::vec2 &a,
        const glm::vec2 &b,
        const glm::vec2 &c)
    {
        auto cross2D = [](const glm::vec2 &v1,
                          const glm::vec2 &v2)
        {
            return v1.x * v2.y - v1.y * v2.x;
        };

        const float d1 = cross2D(b - a, p - a);
        const float d2 = cross2D(c - b, p - b);
        const float d3 = cross2D(a - c, p - c);

        constexpr float epsilon = 0.001f;

        const bool hasNegative =
            d1 < -epsilon || d2 < -epsilon || d3 < -epsilon;

        const bool hasPositive =
            d1 > epsilon || d2 > epsilon || d3 > epsilon;

        return !(hasNegative && hasPositive);
    }
}

bool isMouseOverExtrudeGizmo(
    const glm::vec2 &mouse,
    const ExtrudeGizmoProjection &gizmo)
{
    if (!gizmo.visible)
        return false;

    // Aceita qualquer ponto dentro da face triangular.
    if (isPointInsideTriangle(
            mouse,
            gizmo.screenStart,
            gizmo.screenFaceB,
            gizmo.screenFaceC))
    {
        return true;
    }

    // Também aceita a haste e a ponta da seta.
    constexpr float hitTolerance = 12.0f;

    return distanceToExtrudeGizmo(mouse, gizmo) <= hitTolerance;
}

bool buildScaleGizmo(
    const Window &window,
    const Camera &camera,
    float fovDegrees,
    const Scene::Object &object,
    ScaleGizmoProjection &result)
{
    const BoundingBox *bounds = object.boundingBox();

    if (!bounds || !bounds->valid)
        return false;

    const int width = window.getWidth();
    const int height = window.getHeight();

    if (width <= 0 || height <= 0)
        return false;

    const glm::vec3 localCenter =
        (bounds->min + bounds->max) * 0.5f;

    const glm::mat4 objectMatrix = object.transform.matrix();

    const glm::vec3 worldCenter = glm::vec3(
        objectMatrix * glm::vec4(localCenter, 1.0f));

    const glm::vec3 forward = glm::normalize(camera.orientation);

    const float depth = glm::dot(
        worldCenter - camera.position,
        forward);

    // Não desenha o gizmo atrás da câmera.
    if (depth <= 0.05f)
        return false;

    const glm::mat4 viewProjection = camera.viewProjectionMatrix(fovDegrees);

    if (!projectWorldToScreen(
            worldCenter,
            viewProjection,
            width,
            height,
            result.center))
    {
        return false;
    }

    // Posições das três alças nas extremidades da Bounding Box.
    const glm::vec3 localHandlePositions[3] =
        {
            glm::vec3(bounds->max.x, localCenter.y, localCenter.z),
            glm::vec3(localCenter.x, bounds->max.y, localCenter.z),
            glm::vec3(localCenter.x, localCenter.y, bounds->max.z)};

    const glm::vec3 colors[3] =
        {
            glm::vec3(1.0f, 0.20f, 0.20f),
            glm::vec3(0.20f, 1.0f, 0.20f),
            glm::vec3(0.20f, 0.45f, 1.0f)};

    const char *labels[3] = {"X", "Y", "Z"};

    for (int i = 0; i < 3; ++i)
    {
        ScaleHandleProjection &handle = result.handles[i];

        handle.worldPosition = glm::vec3(
            objectMatrix * glm::vec4(localHandlePositions[i], 1.0f));

        handle.color = colors[i];
        handle.label = labels[i];

        handle.visible = projectWorldToScreen(
            handle.worldPosition,
            viewProjection,
            width,
            height,
            handle.screenPosition);
    }

    return true;
}

// Calcula a posição das três setas do gizmo na tela.
bool buildMoveGizmo(
    const Window &window,
    const Camera &camera,
    float fovDegrees,
    const glm::vec3 &position,
    MoveGizmoProjection &result)
{
    const int width = window.getWidth();
    const int height = window.getHeight();

    if (width <= 0 || height <= 0)
        return false;

    const glm::vec3 forward = glm::normalize(camera.orientation);

    const float depth = glm::dot(
        position - camera.position,
        forward);

    // Não desenha o gizmo atrás da câmera.
    if (depth <= 0.05f)
        return false;

    const glm::mat4 viewProjection = camera.viewProjectionMatrix(fovDegrees);

    if (!projectWorldToScreen(
            position,
            viewProjection,
            width,
            height,
            result.center))
    {
        return false;
    }

    // Mantém o gizmo com tamanho visual aproximadamente constante.
    constexpr float desiredPixelLength = 85.0f;

    const float worldLength = std::clamp(
        2.0f * depth *
            std::tan(glm::radians(fovDegrees) * 0.5f) *
            desiredPixelLength / static_cast<float>(height),
        0.05f,
        5.0f);

    const glm::vec3 directions[3] =
        {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)};

    const glm::vec3 colors[3] =
        {
            glm::vec3(1.0f, 0.20f, 0.20f),
            glm::vec3(0.20f, 1.0f, 0.20f),
            glm::vec3(0.20f, 0.45f, 1.0f)};

    const char *labels[3] = {"X", "Y", "Z"};

    for (int i = 0; i < 3; ++i)
    {
        GizmoAxisProjection &axis = result.axes[i];

        axis.worldDirection = directions[i];
        axis.color = colors[i];
        axis.label = labels[i];

        const glm::vec3 worldEnd =
            position + directions[i] * worldLength;

        if (!projectWorldToScreen(
                worldEnd,
                viewProjection,
                width,
                height,
                axis.screenEnd))
        {
            continue;
        }

        axis.screenStart = result.center;

        const glm::vec2 screenDelta =
            axis.screenEnd - axis.screenStart;

        const float screenLength = glm::length(screenDelta);

        if (screenLength < 6.0f)
            continue;

        axis.screenDirection = screenDelta / screenLength;
        axis.worldPerPixel = worldLength / screenLength;
        axis.visible = true;
    }

    return true;
}

// Calcula a posição das três direçoes pelo circulo do gizmo na tela.
bool buildRotateGizmo(
    const Window &window,
    const Camera &camera,
    float fovDegrees,
    const Scene::Object &object,
    RotateGizmoProjection &result)
{
    const int width = window.getWidth();
    const int height = window.getHeight();

    if (width <= 0 || height <= 0)
        return false;

    const glm::vec3 forward = glm::normalize(camera.orientation);

    const float depth = glm::dot(
        object.transform.position - camera.position,
        forward);

    // Não desenha o gizmo atrás da câmera.
    if (depth <= 0.05f)
        return false;

    const glm::mat4 viewProjection = camera.viewProjectionMatrix(fovDegrees);

    if (!projectWorldToScreen(
            object.transform.position,
            viewProjection,
            width,
            height,
            result.center))
    {
        return false;
    }
    constexpr float desiredPixelLength = 85.0f;

    // Define o tamanho dos círculos na tela.
    const float worldRadius = std::clamp(
        2.0f * depth *
            std::tan(glm::radians(fovDegrees) * 0.5f) *
            desiredPixelLength / static_cast<float>(height),
        0.05f,
        5.0f);

    constexpr int ringSegments = 64;
    constexpr float twoPi = 6.28318530718f;

    const glm::vec3 directions[3] =
        {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)};

    const glm::vec3 basisUs[3] =
        {
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)};

    const glm::vec3 basisVs[3] =
        {
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)};

    const glm::vec3 colors[3] =
        {
            glm::vec3(1.0f, 0.20f, 0.20f),
            glm::vec3(0.20f, 1.0f, 0.20f),
            glm::vec3(0.20f, 0.45f, 1.0f)};

    const char *labels[3] = {"X", "Y", "Z"};

    for (int i = 0; i < 3; ++i)
    {
        GizmoRingProjection &ring = result.rings[i];

        ring.screenPoints.clear();

        ring.worldAxis = directions[i];
        ring.basisU = basisUs[i];
        ring.basisV = basisVs[i];
        ring.color = colors[i];
        ring.label = labels[i];

        // Calcula os pontos que formam o círculo.
        for (int j = 0; j < ringSegments; ++j)
        {
            const float angle =
                twoPi * static_cast<float>(j) / ringSegments;

            const glm::vec3 worldPoint =
                object.transform.position +
                worldRadius * (ring.basisU * std::cos(angle) +
                               ring.basisV * std::sin(angle));

            glm::vec2 screenPoint;

            if (!projectWorldToScreen(
                    worldPoint,
                    viewProjection,
                    width,
                    height,
                    screenPoint))
            {
                ring.screenPoints.clear();
                break;
            }

            ring.screenPoints.push_back(screenPoint);
        }
    }

    return true;
}

namespace
{
    // Calcula a distância entre o cursor e uma seta.
    float distanceToAxis(
        const glm::vec2 &mouse,
        const GizmoAxisProjection &axis)
    {
        const glm::vec2 segment =
            axis.screenEnd - axis.screenStart;

        const float segmentLengthSquared =
            glm::dot(segment, segment);

        if (segmentLengthSquared <= 0.0001f)
            return std::numeric_limits<float>::max();

        const float t = std::clamp(
            glm::dot(mouse - axis.screenStart, segment) /
                segmentLengthSquared,
            0.0f,
            1.0f);

        const glm::vec2 closestPoint =
            axis.screenStart + segment * t;

        return glm::length(mouse - closestPoint);
    }

    // Calcula a distância entre o cursor e uma seta.
    // Calcula a distância entre o cursor e um anel.
    float distanceToRing(
        const glm::vec2 &mouse,
        const GizmoRingProjection &ring)
    {
        if (ring.screenPoints.size() < 2)
            return std::numeric_limits<float>::max();

        float closestDistance = std::numeric_limits<float>::max();

        for (int i = 0; i < static_cast<int>(ring.screenPoints.size()); ++i)
        {
            const glm::vec2 &start = ring.screenPoints[i];

            const glm::vec2 &end =
                ring.screenPoints[(i + 1) % ring.screenPoints.size()];

            const glm::vec2 segment = end - start;

            const float segmentLengthSquared =
                glm::dot(segment, segment);

            if (segmentLengthSquared <= 0.0001f)
                continue;

            const float t = std::clamp(
                glm::dot(mouse - start, segment) / segmentLengthSquared,
                0.0f,
                1.0f);

            const glm::vec2 closestPoint =
                start + segment * t;

            const float distance =
                glm::length(mouse - closestPoint);

            if (distance < closestDistance)
                closestDistance = distance;
        }

        return closestDistance;
    }
}

// Retorna o eixo mais próximo do cursor, se estiver dentro da tolerância.
int findGizmoAxis(
    const MoveGizmoProjection &gizmo,
    const glm::vec2 &mouse)
{
    constexpr float hitTolerance = 11.0f;

    int closestAxis = -1;
    float closestDistance = hitTolerance;

    for (int i = 0; i < 3; ++i)
    {
        const GizmoAxisProjection &axis = gizmo.axes[i];

        if (!axis.visible)
            continue;

        const float distance = distanceToAxis(mouse, axis);

        if (distance <= closestDistance)
        {
            closestDistance = distance;
            closestAxis = i;
        }
    }

    return closestAxis;
}

// Retorna a alça de escala mais próxima do cursor.
int findScaleHandle(
    const ScaleGizmoProjection &gizmo,
    const glm::vec2 &mouse)
{
    constexpr float hitTolerance = 12.0f;

    int closestHandle = -1;
    float closestDistance = hitTolerance;

    for (int i = 0; i < 3; ++i)
    {
        const ScaleHandleProjection &handle = gizmo.handles[i];

        if (!handle.visible)
            continue;

        const float distance = glm::length(
            mouse - handle.screenPosition);

        if (distance <= closestDistance)
        {
            closestDistance = distance;
            closestHandle = i;
        }
    }

    return closestHandle;
}

// Retorna o anel mais próximo do cursor, se estiver dentro da tolerância.
int findGizmoRing(
    const RotateGizmoProjection &gizmo,
    const glm::vec2 &mouse)
{
    constexpr float hitTolerance = 11.0f;

    int closestRing = -1;
    float closestDistance = hitTolerance;

    for (int i = 0; i < 3; ++i)
    {
        const GizmoRingProjection &ring = gizmo.rings[i];

        const float distance = distanceToRing(mouse, ring);

        if (distance <= closestDistance)
        {
            closestDistance = distance;
            closestRing = i;
        }
    }

    return closestRing;
}
