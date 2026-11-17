#pragma once

#include <glm/glm.hpp>

#include "engine/graphics/camera.h"
#include "engine/platform/window.h"

// Geometria para apontar com o mouse para objetos 3D: raios, interseções e projeção na tela.

// Converte a posição do cursor em um raio que parte da câmera.
glm::vec3 createMouseRay(
    const Window &window,
    const Camera &camera,
    float fovDegrees);

// Calcula a interseção de um raio com um plano; retorna false se forem paralelos ou se o plano estiver atrás do raio.
bool intersectRayPlane(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection,
    const glm::vec3 &planePoint,
    const glm::vec3 &planeNormal,
    glm::vec3 &intersection);

// Retorna true se o raio atinge a esfera à sua frente.
bool intersectRaySphere(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection,
    const glm::vec3 &sphereCenter,
    float sphereRadius);

// Converte uma posição 3D em coordenadas de tela.
bool projectWorldToScreen(
    const glm::vec3 &worldPosition,
    const glm::mat4 &viewProjection,
    int width,
    int height,
    glm::vec2 &screenPosition);
