#include "application.h"

#include <algorithm>
#include <iostream>
#include <array>
#include <cmath>

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
        }

        if (window.keyPressed(GLFW_KEY_0))
        {
            scene.showAll();
            selectedModelName = "Todos";
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

            // Primeiro verifica se o clique atingiu uma seta do gizmo.
            if (selected &&
                activeTransformTool == TransformTool::Move)
            {
                MoveGizmoProjection gizmo;

                if (buildMoveGizmo(
                        window, camera, settings.fov,
                        *selected, gizmo))
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
            }

            // Se não clicou no gizmo, tenta selecionar outro objeto.
            if (!startedDragging)
            {
                draggingGizmo = false;
                draggedAxis = -1;

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

    if (editingMode &&
        !menu.isOpen() &&
        window.mouseClicked(GLFW_MOUSE_BUTTON_LEFT))
    {
        // Ignora cliques na barra de ferramentas.
        const float buttonSize = 46.0f;
        const float gap = 7.0f;
        const float margin = 20.0f;

        const float toolbarWidth = buttonSize * 3.0f + gap * 2.0f;
        const float toolbarLeft =
            static_cast<float>(window.getWidth()) - toolbarWidth - margin;

        const float mouseX = static_cast<float>(window.getMouseX());
        const float mouseY = static_cast<float>(window.getMouseY());

        const bool clickedToolbar =
            mouseX >= toolbarLeft &&
            mouseX <= toolbarLeft + toolbarWidth &&
            mouseY >= margin &&
            mouseY <= margin + buttonSize;

        if (!clickedToolbar)
        {
            const glm::vec3 rayDirection =
                createMouseRay(window, camera, settings.fov);

            Scene::Object *selected =
                scene.pickObject(camera.position, rayDirection);

            if (selected)
                std::cout << "Objeto selecionado: "
                          << selected->name << std::endl;
            else
                std::cout << "Nenhum objeto selecionado." << std::endl;
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

void Application::renderUI()
{
    ui.begin(window);
    hud.draw(ui, camera.position);
    hud.textmodel(ui, selectedModelName);
    menu.draw(window, ui, settings);

    // Mostra as ferramentas somente no modo de edição.
    // Mostra o gizmo e as ferramentas somente no modo de edição.
    if (editingMode && !menu.isOpen())
    {
        if (activeTransformTool == TransformTool::Move)
            drawMoveGizmo();

        drawTransformToolbar();
    }

    ui.end();
}