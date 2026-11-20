#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "engine/graphics/camera/camera.h"
#include "engine/platform/window.h"
#include "engine/scene/scene.h"

// Geometria dos gizmos de edição (mover, rotacionar, escalar e extrudar) projetada na tela:
// onde cada manipulador aparece e qual deles está sob o mouse.

struct GizmoAxisProjection
{
    glm::vec3 worldDirection{0.0f};
    glm::vec2 screenStart{0.0f};
    glm::vec2 screenEnd{0.0f};
    glm::vec2 screenDirection{0.0f};

    glm::vec3 color{1.0f};
    const char *label = "";

    float worldPerPixel = 0.0f;
    bool visible = false;
};

struct GizmoRingProjection
{
    glm::vec3 worldAxis{0.0f};

    glm::vec3 basisU{0.0f};
    glm::vec3 basisV{0.0f};

    glm::vec3 color{1.0f};

    std::vector<glm::vec2> screenPoints;

    const char *label = "";
};

struct RotateGizmoProjection
{
    glm::vec2 center{0.0f};
    std::array<GizmoRingProjection, 3> rings;
};

struct MoveGizmoProjection
{
    glm::vec2 center{0.0f};
    std::array<GizmoAxisProjection, 3> axes;
};

struct ScaleHandleProjection
{
    glm::vec3 worldPosition{0.0f};
    glm::vec2 screenPosition{0.0f};

    glm::vec3 color{1.0f};

    const char *label = "";

    bool visible = false;
};

struct ScaleGizmoProjection
{
    glm::vec2 center{0.0f};

    std::array<ScaleHandleProjection, 3> handles;
};

struct ExtrudeGizmoProjection
{
    glm::vec3 worldCenter{0.0f};
    glm::vec3 worldNormal{0.0f};

    glm::vec2 screenStart{0.0f};
    glm::vec2 screenEnd{0.0f};
    glm::vec2 screenDirection{0.0f};

    glm::vec2 screenFaceB{0.0f};
    glm::vec2 screenFaceC{0.0f};

    float worldLength = 0.0f;
    float worldPerPixel = 0.0f;

    bool visible = false;
};

bool buildExtrudeGizmoProjection(
    const Window &window,
    const Camera &camera,
    const Scene &scene,
    const Scene::Object *selectedObject,
    const Scene::FaceSelection &selectedFace,
    ExtrudeGizmoProjection &result);

bool isMouseOverExtrudeGizmo(
    const glm::vec2 &mouse,
    const ExtrudeGizmoProjection &gizmo);

bool buildScaleGizmo(
    const Window &window,
    const Camera &camera,
    const Scene::Object &object,
    ScaleGizmoProjection &result);

// Calcula a posição das três setas do gizmo na tela.
bool buildMoveGizmo(
    const Window &window,
    const Camera &camera,
    const glm::vec3 &position,
    MoveGizmoProjection &result);

// Calcula a posição das três direçoes pelo circulo do gizmo na tela.
bool buildRotateGizmo(
    const Window &window,
    const Camera &camera,
    const Scene::Object &object,
    RotateGizmoProjection &result);

// Retorna o eixo mais próximo do cursor, se estiver dentro da tolerância.
int findGizmoAxis(
    const MoveGizmoProjection &gizmo,
    const glm::vec2 &mouse);

// Retorna a alça de escala mais próxima do cursor.
int findScaleHandle(
    const ScaleGizmoProjection &gizmo,
    const glm::vec2 &mouse);

// Retorna o anel mais próximo do cursor, se estiver dentro da tolerância.
int findGizmoRing(
    const RotateGizmoProjection &gizmo,
    const glm::vec2 &mouse);
