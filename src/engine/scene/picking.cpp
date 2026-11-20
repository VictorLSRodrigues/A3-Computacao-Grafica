#include "engine/scene/picking.h"

#include <cmath>

// Converte a posição do cursor em um raio que parte da câmera.
glm::vec3 createMouseRay(
    const Window &window,
    const Camera &camera)
{
    const float width = static_cast<float>(window.getWidth());
    const float height = static_cast<float>(window.getHeight());

    if (width <= 0.0f || height <= 0.0f)
        return glm::vec3(0.0f);

    // Converte as coordenadas da tela para coordenadas normalizadas.
    const float ndcX =
        2.0f * static_cast<float>(window.getMouseX()) / width - 1.0f;

    const float ndcY =
        1.0f - 2.0f * static_cast<float>(window.getMouseY()) / height;

    const float aspect = width / height;
    const float tanHalfFov = std::tan(glm::radians(camera.fov) * 0.5f);

    const glm::vec3 forward = glm::normalize(camera.orientation);
    const glm::vec3 right =
        glm::normalize(glm::cross(forward, camera.up));
    const glm::vec3 cameraUp =
        glm::normalize(glm::cross(right, forward));

    return glm::normalize(
        forward +
        right * (ndcX * aspect * tanHalfFov) +
        cameraUp * (ndcY * tanHalfFov));
}

// Calcula a interseção de um raio com um plano; retorna false se forem paralelos ou se o plano estiver atrás do raio.
bool intersectRayPlane(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection,
    const glm::vec3 &planePoint,
    const glm::vec3 &planeNormal,
    glm::vec3 &intersection)
{
    const float denominator =
        glm::dot(rayDirection, planeNormal);

    if (std::abs(denominator) < 0.000001f)
        return false;

    const float distance =
        glm::dot(planePoint - rayOrigin, planeNormal) /
        denominator;

    if (distance < 0.0f)
        return false;

    intersection = rayOrigin + rayDirection * distance;

    return true;
}

// Retorna true se o raio atinge a esfera à sua frente.
bool intersectRaySphere(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection,
    const glm::vec3 &sphereCenter,
    float sphereRadius)
{
    const glm::vec3 offset = rayOrigin - sphereCenter;
    const float b = glm::dot(offset, rayDirection);
    const float c = glm::dot(offset, offset) - sphereRadius * sphereRadius;
    const float discriminant = b * b - c;

    if (discriminant < 0.0f)
        return false;

    return -b + std::sqrt(discriminant) >= 0.0f;
}

// Converte uma posição 3D em coordenadas de tela.
bool projectWorldToScreen(
    const glm::vec3 &worldPosition,
    const glm::mat4 &viewProjection,
    int width,
    int height,
    glm::vec2 &screenPosition)
{
    const glm::vec4 clipPosition =
        viewProjection * glm::vec4(worldPosition, 1.0f);

    if (clipPosition.w <= 0.001f)
        return false;

    const glm::vec3 ndc =
        glm::vec3(clipPosition) / clipPosition.w;

    screenPosition.x = (ndc.x + 1.0f) * 0.5f * width;
    screenPosition.y = (1.0f - ndc.y) * 0.5f * height;

    return true;
}
