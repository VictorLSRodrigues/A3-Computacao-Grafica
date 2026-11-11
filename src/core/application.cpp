#include "application.h"

#include <algorithm>
#include <iostream>
#include <array>
#include <cmath>
#include <vector>
#include <limits>

#include "scene/world.h"

namespace
{
    // Converte a posição do cursor em um raio que parte da câmera.
    glm::vec3 createMouseRay(
        const Window &window,
        const Camera &camera,
        float fovDegrees)
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
        const float tanHalfFov = std::tan(glm::radians(fovDegrees) * 0.5f);

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

    // Calcula a interseção do raio do mouse com o plano do anel.
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

    // Calcula a posição das três setas do gizmo na tela.
    bool buildMoveGizmo(
        const Window &window,
        const Camera &camera,
        float fovDegrees,
        const Scene::Object &object,
        MoveGizmoProjection &result)
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

        const glm::mat4 view = glm::lookAt(
            camera.position,
            camera.position + camera.orientation,
            camera.up);

        const glm::mat4 projection = glm::perspective(
            glm::radians(fovDegrees),
            static_cast<float>(width) / height,
            0.1f,
            1000.0f);

        const glm::mat4 viewProjection = projection * view;

        if (!projectWorldToScreen(
                object.transform.position,
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
                object.transform.position + directions[i] * worldLength;

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

        const glm::mat4 view = glm::lookAt(
            camera.position,
            camera.position + camera.orientation,
            camera.up);

        const glm::mat4 projection = glm::perspective(
            glm::radians(fovDegrees),
            static_cast<float>(width) / height,
            0.1f,
            1000.0f);

        const glm::mat4 viewProjection = projection * view;

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

}

bool Application::init()
{
    if (!window.create("OpenGL - RotorGrafico"))
        return false;

    if (!renderer.init())
        return false;

    if (!light.create())
    {
        std::cout << "Erro ao criar a malha da luz." << std::endl;
        return false;
    }

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI!" << std::endl;
        return false;
    }

    return buildWorld(scene);
}

void Application::run()
{
    double lastFrame = glfwGetTime();

    while (!window.shouldClose())
    {
        const double currentFrame = glfwGetTime();

        // Evita um salto grande de movimento se um quadro demorar demais.
        const float deltaTime = static_cast<float>(std::min(currentFrame - lastFrame, 0.1));
        lastFrame = currentFrame;

        update(deltaTime);
        renderer.render(scene, light, camera, settings);
        renderUI();

        window.swapBuffers();
        window.pollEvents();
    }
}

void Application::update(float deltaTime)
{
    hud.update(window, deltaTime);
    menu.update(window);

    // A câmera só recebe controles quando o menu está fechado.
    if (menu.isOpen())
    {
        window.setCursorMode(Window::CursorMode::Normal);
    }
    else
    {
        // Alterna entre o modo de navegação e o modo de edição.
        if (window.keyPressed(GLFW_KEY_TAB))
        {
            editingMode = !editingMode;

            if (editingMode)
                std::cout << "Modo de edicao ativado!" << std::endl;
            else
                std::cout << "Modo de navegacao ativado!" << std::endl;
        }

        camera.processInput(window, deltaTime);

        if (!editingMode)
        {
            if (window.keyPressed(GLFW_KEY_1))
            {
                if (scene.showOnly("macaco"))
                    selectedModelName = "macaco";
                else
                    std::cout << "Objeto macaco nao encontrado na cena." << std::endl;
            }
            else if (window.keyPressed(GLFW_KEY_2))
            {
                if (scene.showOnly("multi"))
                    selectedModelName = "multi";
                else
                    std::cout << "Objeto multi nao encontrado na cena." << std::endl;
            }
            else if (window.keyPressed(GLFW_KEY_3))
            {
                if (scene.showOnly("pyramid"))
                    selectedModelName = "pyramid";
                else
                    std::cout << "Objeto pyramid nao encontrado na cena." << std::endl;
            }
            else if (window.keyPressed(GLFW_KEY_0))
            {
                scene.showAll();
                selectedModelName = "Todos";
            }
        }
    }

    // Copia para a luz e para a câmera os ajustes escolhidos no menu.
    light.position = settings.lightPosition;
    light.target = settings.lightTarget;

    camera.walkSpeed = settings.walkSpeed;
    camera.sprintSpeed = settings.sprintSpeed;
    camera.sensitivity = settings.mouseSensitivity;
    camera.setViewportSize(window.getWidth(), window.getHeight());

    // Seleção e movimentação de objetos no modo de edição.
    const bool editingAvailable = editingMode && !menu.isOpen();

    if (!editingAvailable)
    {
        draggingGizmo = false;
        draggedAxis = -1;

        draggingRotationGizmo = false;
        rotatingAxis = -1;
    }

    if (editingAvailable &&
        window.mouseClicked(GLFW_MOUSE_BUTTON_LEFT))
    {
        // Ignora cliques na barra de ferramentas.
        const float buttonSize = 46.0f;
        const float gap = 7.0f;
        const float margin = 20.0f;

        const float toolbarWidth =
            buttonSize * 3.0f + gap * 2.0f;

        const float toolbarLeft =
            static_cast<float>(window.getWidth()) -
            toolbarWidth - margin;

        const glm::vec2 mouse(
            static_cast<float>(window.getMouseX()),
            static_cast<float>(window.getMouseY()));

        const bool clickedToolbar =
            mouse.x >= toolbarLeft &&
            mouse.x <= toolbarLeft + toolbarWidth &&
            mouse.y >= margin &&
            mouse.y <= margin + buttonSize;

        if (!clickedToolbar)
        {
            bool startedDragging = false;

            Scene::Object *selected = scene.getSelectedObject();

            if (selected)
            {
                switch (activeTransformTool)
                {
                case TransformTool::Move:
                {
                    // Primeiro verifica se o clique atingiu uma seta do gizmo.
                    MoveGizmoProjection gizmo;

                    if (buildMoveGizmo(
                            window,
                            camera,
                            settings.fov,
                            *selected,
                            gizmo))
                    {
                        const int axis = findGizmoAxis(gizmo, mouse);

                        if (axis >= 0)
                        {
                            const GizmoAxisProjection &selectedAxis =
                                gizmo.axes[axis];

                            draggingGizmo = true;
                            draggedAxis = axis;

                            dragStartPosition =
                                selected->transform.position;

                            draggedWorldAxis =
                                selectedAxis.worldDirection;

                            dragStartMouse = mouse;

                            dragScreenDirection =
                                selectedAxis.screenDirection;

                            dragWorldPerPixel =
                                selectedAxis.worldPerPixel;

                            startedDragging = true;

                            std::cout
                                << "Movendo no eixo "
                                << selectedAxis.label
                                << std::endl;
                        }
                    }

                    break;
                }

                case TransformTool::Rotate:
                {
                    // Primeiro verifica se o clique atingiu um anel do gizmo.
                    RotateGizmoProjection gizmo;

                    if (buildRotateGizmo(
                            window,
                            camera,
                            settings.fov,
                            *selected,
                            gizmo))
                    {
                        const int ringIndex =
                            findGizmoRing(gizmo, mouse);

                        if (ringIndex >= 0)
                        {
                            const GizmoRingProjection &selectedRing =
                                gizmo.rings[ringIndex];

                            const glm::vec3 rayDirection =
                                createMouseRay(window, camera, settings.fov);

                            glm::vec3 intersection;

                            if (intersectRayPlane(
                                    camera.position,
                                    rayDirection,
                                    selected->transform.position,
                                    selectedRing.worldAxis,
                                    intersection))
                            {
                                // Guarda a direção inicial do mouse no plano do anel.
                                rotationStartVector = glm::normalize(
                                    intersection - selected->transform.position);

                                // Guarda a rotação original do objeto.
                                rotationStart = selected->transform.rotation;

                                // Guarda o eixo escolhido.
                                rotatingWorldAxis = selectedRing.worldAxis;
                                rotatingAxis = ringIndex;

                                rotationAccumulatedAngle = 0.0f;

                                draggingRotationGizmo = true;
                                startedDragging = true;

                                std::cout
                                    << "Rotacionando no eixo "
                                    << selectedRing.label
                                    << std::endl;
                            }
                        }
                    }

                    break;
                }

                case TransformTool::Scale:
                {
                    // Ferramenta de escala será implementada depois.
                    break;
                }
                }
            }

            // Se não clicou no gizmo, tenta selecionar outro objeto.
            if (!startedDragging)
            {
                draggingGizmo = false;
                draggedAxis = -1;

                draggingRotationGizmo = false;
                rotatingAxis = -1;

                const glm::vec3 rayDirection =
                    createMouseRay(window, camera, settings.fov);

                selected = scene.pickObject(
                    camera.position,
                    rayDirection);

                if (selected)
                    std::cout << "Objeto selecionado: "
                              << selected->name << std::endl;
                else
                    std::cout << "Nenhum objeto selecionado."
                              << std::endl;
            }
        }
    }

    // Atualiza a posição enquanto o botão esquerdo está pressionado.
    if (draggingGizmo)
    {
        if (editingAvailable &&
            window.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            Scene::Object *selected = scene.getSelectedObject();

            if (selected && draggedAxis >= 0)
            {
                const glm::vec2 currentMouse(
                    static_cast<float>(window.getMouseX()),
                    static_cast<float>(window.getMouseY()));

                // Considera apenas o deslocamento na direção projetada do eixo.
                const float pixelDelta = glm::dot(
                    currentMouse - dragStartMouse,
                    dragScreenDirection);

                const float worldDelta =
                    pixelDelta * dragWorldPerPixel;

                selected->transform.position =
                    dragStartPosition +
                    draggedWorldAxis * worldDelta;
            }
            else
            {
                draggingGizmo = false;
                draggedAxis = -1;
            }
        }
        else
        {
            // Soltar o botão encerra a movimentação.
            draggingGizmo = false;
            draggedAxis = -1;
        }
    }

if (draggingRotationGizmo)
{
    if (editingAvailable &&
        window.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        Scene::Object *selected = scene.getSelectedObject();

        if (selected && rotatingAxis >= 0)
        {
            const glm::vec3 rayDirection =
                createMouseRay(window, camera, settings.fov);

            glm::vec3 intersection;

            if (intersectRayPlane(
                    camera.position,
                    rayDirection,
                    selected->transform.position,
                    rotatingWorldAxis,
                    intersection))
            {
                glm::vec3 currentVector =
                    intersection - selected->transform.position;

                if (glm::length(currentVector) > 0.0001f)
                {
                    currentVector = glm::normalize(currentVector);

                    // Calcula o ângulo entre a posição anterior e a atual.
                    const float deltaAngle = std::atan2(
                        glm::dot(
                            rotatingWorldAxis,
                            glm::cross(rotationStartVector, currentVector)),
                        glm::dot(rotationStartVector, currentVector));

                    rotationAccumulatedAngle +=
                        glm::degrees(deltaAngle);

                    // Aplica a rotação ao eixo selecionado.
                    selected->transform.rotation[rotatingAxis] =
                        rotationStart[rotatingAxis] +
                        rotationAccumulatedAngle;

                    // Atualiza o vetor para o próximo quadro.
                    rotationStartVector = currentVector;
                }
            }
        }
        else
        {
            draggingRotationGizmo = false;
            rotatingAxis = -1;
        }
    }
    else
    {
        // Soltar o botão encerra a rotação.
        draggingRotationGizmo = false;
        rotatingAxis = -1;
    }
    }

    scene.update(deltaTime);
}

// Desenha as ferramentas de transformação no canto superior direito.
void Application::drawTransformToolbar()
{
    const float buttonSize = 46.0f;
    const float gap = 7.0f;
    const float margin = 20.0f;

    const float totalWidth = buttonSize * 3.0f + gap * 2.0f;

    const float x = static_cast<float>(window.getWidth()) - totalWidth - margin;
    const float y = margin;

    // Botão Mover.
    if (ui.iconButton(
            UI::Icon::Move,
            "Mover",
            x, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Move))
    {
        activeTransformTool = TransformTool::Move;
    }

    // Botão Rotacionar.
    if (ui.iconButton(
            UI::Icon::Rotate,
            "Rotacionar",
            x + buttonSize + gap, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Rotate))
    {
        activeTransformTool = TransformTool::Rotate;
    }

    // Botão Escalar.
    if (ui.iconButton(
            UI::Icon::Scale,
            "Escalar",
            x + (buttonSize + gap) * 2.0f, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Scale))
    {
        activeTransformTool = TransformTool::Scale;
    }
}

void Application::drawMoveGizmo()
{
    Scene::Object *selected = scene.getSelectedObject();

    if (!selected)
        return;

    MoveGizmoProjection gizmo;

    if (!buildMoveGizmo(
            window,
            camera,
            settings.fov,
            *selected,
            gizmo))
    {
        return;
    }

    // Pequeno centro do manipulador.
    ui.drawRect(
        gizmo.center.x - 4.0f,
        gizmo.center.y - 4.0f,
        8.0f,
        8.0f,
        0.90f, 0.90f, 0.90f);

    for (const GizmoAxisProjection &axis : gizmo.axes)
    {
        if (!axis.visible)
            continue;

        const float r = axis.color.r;
        const float g = axis.color.g;
        const float b = axis.color.b;

        // Corpo da seta.
        ui.drawLine(
            axis.screenStart.x,
            axis.screenStart.y,
            axis.screenEnd.x,
            axis.screenEnd.y,
            4.0f,
            r, g, b);

        // Ponta da seta.
        const glm::vec2 perpendicular(
            -axis.screenDirection.y,
            axis.screenDirection.x);

        const glm::vec2 arrowBack =
            axis.screenEnd - axis.screenDirection * 12.0f;

        const glm::vec2 arrowLeft =
            arrowBack + perpendicular * 6.0f;

        const glm::vec2 arrowRight =
            arrowBack - perpendicular * 6.0f;

        ui.drawLine(
            axis.screenEnd.x,
            axis.screenEnd.y,
            arrowLeft.x,
            arrowLeft.y,
            3.0f,
            r, g, b);

        ui.drawLine(
            axis.screenEnd.x,
            axis.screenEnd.y,
            arrowRight.x,
            arrowRight.y,
            3.0f,
            r, g, b);

        // Identificação do eixo.
        ui.drawText(
            axis.label,
            axis.screenEnd.x + 7.0f,
            axis.screenEnd.y - 7.0f,
            0.55f,
            r, g, b);
    }
}

void Application::drawRotateGizmo()
{
    Scene::Object *selected = scene.getSelectedObject();

    if (!selected)
        return;

    RotateGizmoProjection gizmo;

    if (!buildRotateGizmo(
            window,
            camera,
            settings.fov,
            *selected,
            gizmo))
    {
        return;
    }

    // Pequeno centro do manipulador.
    ui.drawRect(
        gizmo.center.x - 4.0f,
        gizmo.center.y - 4.0f,
        8.0f,
        8.0f,
        0.90f, 0.90f, 0.90f);

    for (const GizmoRingProjection &ring : gizmo.rings)
    {
        if (ring.screenPoints.size() < 2)
            continue;

        const float r = ring.color.r;
        const float g = ring.color.g;
        const float b = ring.color.b;

        for (int j = 0; j < static_cast<int>(ring.screenPoints.size()); ++j)
        {
            const glm::vec2 &start = ring.screenPoints[j];

            const glm::vec2 &end =
                ring.screenPoints[(j + 1) % ring.screenPoints.size()];

            ui.drawLine(
                start.x,
                start.y,
                end.x,
                end.y,
                2.5f,
                r, g, b);
        }
        // Identificação do eixo.
        const glm::vec2 &labelPosition = ring.screenPoints[8];

        ui.drawText(
            ring.label,
            labelPosition.x + 7.0f,
            labelPosition.y - 7.0f,
            0.55f,
            r, g, b);
    }
}

void Application::renderUI()
{
    // Inicia a interface gráfica.
    ui.begin(window);

    // Desenha o HUD e as informações da cena.
    hud.draw(ui, camera.position);
    hud.textmodel(ui, selectedModelName);

    // Desenha o menu.
    menu.draw(window, ui, settings);

    // Exibe as ferramentas somente no modo de edição
    // e quando o menu estiver fechado.
    if (editingMode && !menu.isOpen())
    {
        // Desenha a ferramenta de transformação selecionada.
        switch (activeTransformTool)
        {
        case TransformTool::Move:
            drawMoveGizmo();
            break;

        case TransformTool::Rotate:
            drawRotateGizmo();
            break;

        case TransformTool::Scale:
            // Ferramenta de escala será implementada depois.
            break;
        }

        // Desenha a barra de ferramentas.
        drawTransformToolbar();
    }

    // Finaliza a interface gráfica.
    ui.end();
}