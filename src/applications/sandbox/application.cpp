#include "application.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "applications/sandbox/editor/gizmo.h"
#include "engine/scene/picking.h"
#include "world.h"

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
    light.color = glm::vec4(settings.lightColor, 1.0f);

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

        draggingScaleGizmo = false;
        scalingAxis = -1;

        if (draggingExtrusionGizmo)
        {
            scene.cancelSelectedFaceExtrusion();

            draggingExtrusionGizmo = false;
            extrusionDistance = 0.0f;
            extrusionChanged = false;
        }
    }

    // Esta parte trata somente o instante do clique.
    if (editingAvailable &&
        window.mouseClicked(GLFW_MOUSE_BUTTON_LEFT))
    {
        const glm::vec2 mouse(
            static_cast<float>(window.getMouseX()),
            static_cast<float>(window.getMouseY()));

        // Ignora cliques sobre a interface (barra de ferramentas, painéis, sliders).
        if (!ui.wantsMouse(window))
        {
            bool startedDragging = false;
            bool handledFaceSelection = false;

            // No modo Extrudar, tenta selecionar diretamente uma face.
            if (activeTransformTool == TransformTool::Extrude)
            {
                bool startedOnGizmo = false;

                // Primeiro verifica se já existe uma face selecionada
                // e se o clique atingiu a seta de extrusão.
                if (scene.getSelectedFace().valid)
                {
                    ExtrudeGizmoProjection gizmo;

                    if (buildExtrudeGizmoProjection(
                            window,
                            camera,
                            settings.fov,
                            scene,
                            gizmo) &&
                        isMouseOverExtrudeGizmo(mouse, gizmo))
                    {
                        if (scene.beginSelectedFaceExtrusion())
                        {
                            draggingGizmo = false;
                            draggedAxis = -1;

                            draggingRotationGizmo = false;
                            rotatingAxis = -1;

                            draggingScaleGizmo = false;
                            scalingAxis = -1;

                            draggingExtrusionGizmo = true;

                            // Guarda exatamente o ponto inicial do clique.
                            extrusionStartMouse = mouse;

                            // Guarda a direção projetada da seta.
                            extrusionScreenDirection =
                                gizmo.screenDirection;

                            // Conversão de pixels para unidades do mundo.
                            extrusionWorldPerPixel =
                                gizmo.worldPerPixel;

                            extrusionDistance = 0.0f;
                            extrusionChanged = false;

                            startedDragging = true;
                            startedOnGizmo = true;
                            handledFaceSelection = true;

                            std::cout
                                << "[EXTRUDE] Arraste iniciado!"
                                << std::endl;
                        }
                    }
                }

                // Se não clicou na seta, tenta selecionar outra face.
                if (!startedOnGizmo)
                {
                    const glm::vec3 rayDirection =
                        createMouseRay(window, camera, settings.fov);

                    const bool faceFound =
                        scene.pickFace(camera.position, rayDirection);

                    handledFaceSelection = true;

                    if (faceFound)
                    {
                        lightSelected = false;

                        const Scene::Object *faceObject =
                            scene.getSelectedObject();

                        const Scene::FaceSelection &face =
                            scene.getSelectedFace();

                        std::cout
                            << "[EXTRUDE] Face selecionada!"
                            << " | Objeto: " << faceObject->name
                            << " | Malha: " << face.meshIndex
                            << " | Face: " << face.faceIndex
                            << std::endl;
                    }
                    else
                    {
                        std::cout
                            << "[EXTRUDE] Nenhuma face encontrada!"
                            << std::endl;
                    }
                }
            }

            Scene::Object *selected =
                lightSelected ? nullptr : scene.getSelectedObject();

            if (selected || lightSelected)
            {
                switch (activeTransformTool)
                {
                case TransformTool::Move:
                {
                    const glm::vec3 &selectedPosition =
                        lightSelected ? light.position : selected->transform.position;

                    // Primeiro verifica se o clique atingiu uma seta do gizmo.
                    MoveGizmoProjection gizmo;

                    if (buildMoveGizmo(
                            window,
                            camera,
                            settings.fov,
                            selectedPosition,
                            gizmo))
                    {
                        const int axis = findGizmoAxis(gizmo, mouse);

                        if (axis >= 0)
                        {
                            const GizmoAxisProjection &selectedAxis =
                                gizmo.axes[axis];

                            draggingRotationGizmo = false;
                            rotatingAxis = -1;
                            draggingScaleGizmo = false;
                            scalingAxis = -1;

                            draggingGizmo = true;
                            draggedAxis = axis;

                            dragStartPosition =
                                selectedPosition;

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
                    if (lightSelected)
                        break;

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

                                draggingGizmo = false;
                                draggedAxis = -1;
                                draggingScaleGizmo = false;
                                scalingAxis = -1;

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
                    if (lightSelected)
                        break;

                    // Primeiro verifica se o clique atingiu uma alça do gizmo.
                    ScaleGizmoProjection gizmo;

                    if (buildScaleGizmo(
                            window,
                            camera,
                            settings.fov,
                            *selected,
                            gizmo))
                    {
                        const int handleIndex = findScaleHandle(gizmo, mouse);

                        if (handleIndex >= 0)
                        {
                            const ScaleHandleProjection &selectedHandle =
                                gizmo.handles[handleIndex];

                            // Calcula a direção projetada da alça.
                            const glm::vec2 handleOffset =
                                selectedHandle.screenPosition - gizmo.center;

                            const float screenLength = glm::length(handleOffset);

                            if (screenLength > 6.0f)
                            {
                                // Guarda o eixo escolhido.
                                scalingAxis = handleIndex;

                                // Guarda a escala original.
                                scaleStart = selected->transform.scale;

                                const BoundingBox *bounds = selected->boundingBox();

                                if (bounds && bounds->valid)
                                {
                                    scalePivotLocal = (bounds->min + bounds->max) * 0.5f;

                                    scalePivotWorld = glm::vec3(
                                        selected->transform.matrix() *
                                        glm::vec4(scalePivotLocal, 1.0f));
                                }

                                // Guarda a posição inicial do mouse.
                                scaleStartMouse = mouse;

                                // Direção do movimento na tela.
                                scaleScreenDirection = glm::normalize(handleOffset);

                                // Sensibilidade da escala por pixel.
                                scaleFactorPerPixel = 1.0f / screenLength;

                                draggingGizmo = false;
                                draggedAxis = -1;
                                draggingRotationGizmo = false;
                                rotatingAxis = -1;

                                draggingScaleGizmo = true;
                                startedDragging = true;

                                std::cout
                                    << "Escalando no eixo "
                                    << selectedHandle.label
                                    << std::endl;
                            }
                        }
                    }

                    break;
                }
                }
            }

            // Se não clicou no gizmo, tenta selecionar outro objeto.
            if (!startedDragging && !handledFaceSelection)
            {
                draggingGizmo = false;
                draggedAxis = -1;

                draggingRotationGizmo = false;
                rotatingAxis = -1;

                draggingScaleGizmo = false;
                scalingAxis = -1;

                const glm::vec3 rayDirection =
                    createMouseRay(window, camera, settings.fov);

                selected = scene.pickObject(
                    camera.position,
                    rayDirection);

                lightSelected = intersectRaySphere(
                    camera.position,
                    rayDirection,
                    light.position,
                    0.12f);

                if (lightSelected)
                    std::cout << "Luz selecionada." << std::endl;
                else if (selected)
                    std::cout << "Objeto selecionado: "
                              << selected->name << std::endl;
                else
                    std::cout << "Nenhum objeto selecionado."
                              << std::endl;
            }
        }
    } // Fim do tratamento do clique.

    // A partir daqui, os três blocos de arraste são executados a cada quadro,
    // e não somente no quadro em que o mouse foi clicado.

    // Atualiza a posição enquanto o botão esquerdo está pressionado.
    if (draggingGizmo)
    {
        if (editingAvailable &&
            window.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            Scene::Object *selected =
                lightSelected ? nullptr : scene.getSelectedObject();

            if ((selected || lightSelected) && draggedAxis >= 0)
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

                const glm::vec3 newPosition =
                    dragStartPosition + draggedWorldAxis * worldDelta;

                if (lightSelected)
                {
                    settings.lightPosition = newPosition;
                    light.position = newPosition;
                }
                else
                {
                    selected->transform.position = newPosition;
                }
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

    // Atualiza a rotação enquanto o botão esquerdo está pressionado.
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

    // Atualiza a escala enquanto o botão esquerdo está pressionado.
    if (draggingScaleGizmo)
    {
        if (editingAvailable &&
            window.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            Scene::Object *selected = scene.getSelectedObject();

            if (selected && scalingAxis >= 0 && scalingAxis < 3)
            {
                // Posição atual do mouse.
                const glm::vec2 currentMouse(
                    static_cast<float>(window.getMouseX()),
                    static_cast<float>(window.getMouseY()));

                const float pixelDelta = glm::dot(
                    currentMouse - scaleStartMouse,
                    scaleScreenDirection);

                // Calcula a variação da escala.
                const float scaleDelta =
                    pixelDelta * scaleFactorPerPixel;

                // Calcula o multiplicador, impedindo escala zero ou negativa.
                const float scaleMultiplier =
                    std::max(0.001f, 1.0f + scaleDelta);

                // Aplica a escala somente no eixo selecionado.
                selected->transform.scale[scalingAxis] =
                    scaleStart[scalingAxis] * scaleMultiplier;
            }
            else
            {
                draggingScaleGizmo = false;
                scalingAxis = -1;
            }
        }
        else
        {
            // Soltar o botão encerra a escala.
            draggingScaleGizmo = false;
            scalingAxis = -1;
        }
    }

    // Atualiza a extrusão enquanto o botão esquerdo está pressionado.
    if (draggingExtrusionGizmo)
    {
        if (editingAvailable &&
            window.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            const glm::vec2 currentMouse(
                static_cast<float>(window.getMouseX()),
                static_cast<float>(window.getMouseY()));

            // Considera somente o movimento na direção da seta.
            const float pixelDelta = glm::dot(
                currentMouse - extrusionStartMouse,
                extrusionScreenDirection);

            // Converte o movimento da tela em distância 3D.
            const float distance =
                pixelDelta * extrusionWorldPerPixel;

            extrusionDistance = distance;

            if (std::abs(distance) > 0.00001f)
            {
                if (scene.updateSelectedFaceExtrusion(distance))
                    extrusionChanged = true;
            }
        }
        else
        {
            // Ao soltar o mouse, confirma ou cancela a operação.
            if (extrusionChanged)
            {
                scene.finishSelectedFaceExtrusion();

                std::cout
                    << "[EXTRUDE] Extrusao confirmada: "
                    << extrusionDistance
                    << std::endl;
            }
            else
            {
                scene.cancelSelectedFaceExtrusion();

                std::cout
                    << "[EXTRUDE] Extrusao cancelada."
                    << std::endl;
            }

            draggingExtrusionGizmo = false;
            extrusionDistance = 0.0f;
            extrusionChanged = false;
        }
    }

    scene.update(deltaTime);
}

// Desenha as ferramentas de transformação no canto superior direito.
void Application::drawTransformToolbar()
{
    const float screenWidth = static_cast<float>(window.getWidth());
    const float margin = std::min(20.0f, screenWidth * 0.04f);
    const float gap = std::min(7.0f, screenWidth * 0.015f);
    const float buttonSize = std::min(
        46.0f,
        std::max(28.0f, (screenWidth - 2.0f * margin - 3.0f * gap) / 4.0f));

    const float totalWidth = buttonSize * 4.0f + gap * 3.0f;

    const float x = std::max(margin, screenWidth - totalWidth - margin);
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

    // Botão Extrudar.
    if (ui.iconButton(
            UI::Icon::Extrude,
            "Extrusão",
            x + (buttonSize + gap) * 3.0f, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Extrude))
    {
        activeTransformTool = TransformTool::Extrude;
    }
}

void Application::drawMoveGizmo()
{
    Scene::Object *selected =
        lightSelected ? nullptr : scene.getSelectedObject();

    if (!selected && !lightSelected)
        return;

    const glm::vec3 &selectedPosition =
        lightSelected ? light.position : selected->transform.position;

    MoveGizmoProjection gizmo;

    if (!buildMoveGizmo(
            window,
            camera,
            settings.fov,
            selectedPosition,
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
    if (lightSelected)
        return;

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

// Desenha o contorno da face selecionada.
void Application::drawSelectedFaceOutline()
{
    const Scene::FaceSelection &selection =
        scene.getSelectedFace();

    if (!selection.valid)
        return;

    glm::vec3 a, b, c;

    if (!scene.getSelectedFacePositions(a, b, c))
        return;

    const int width = window.getWidth();
    const int height = window.getHeight();

    if (width <= 0 || height <= 0)
        return;

    const glm::mat4 viewProjection = camera.viewProjectionMatrix(settings.fov);

    glm::vec2 screenA;
    glm::vec2 screenB;
    glm::vec2 screenC;

    if (!projectWorldToScreen(a, viewProjection, width, height, screenA) ||
        !projectWorldToScreen(b, viewProjection, width, height, screenB) ||
        !projectWorldToScreen(c, viewProjection, width, height, screenC))
    {
        return;
    }

    // Cor laranja da seleção.
    constexpr float orangeR = 1.0f;
    constexpr float orangeG = 0.40f;
    constexpr float orangeB = 0.05f;

    ui.drawLine(
        screenA.x, screenA.y,
        screenB.x, screenB.y,
        4.0f, orangeR, orangeG, orangeB);

    ui.drawLine(
        screenB.x, screenB.y,
        screenC.x, screenC.y,
        4.0f, orangeR, orangeG, orangeB);

    ui.drawLine(
        screenC.x, screenC.y,
        screenA.x, screenA.y,
        4.0f, orangeR, orangeG, orangeB);
}

void Application::drawExtrudeGizmo()
{
    ExtrudeGizmoProjection gizmo;

    if (!buildExtrudeGizmoProjection(
            window,
            camera,
            settings.fov,
            scene,
            gizmo))
    {
        return;
    }

    const glm::vec2 &start = gizmo.screenStart;
    const glm::vec2 &end = gizmo.screenEnd;
    const glm::vec2 &direction = gizmo.screenDirection;

    // Corpo da seta.
    ui.drawLine(
        start.x, start.y,
        end.x, end.y,
        4.0f,
        1.0f, 0.65f, 0.10f);

    // Ponta da seta.
    const glm::vec2 perpendicular(
        -direction.y,
        direction.x);

    const glm::vec2 arrowBack =
        end - direction * 13.0f;

    const glm::vec2 arrowLeft =
        arrowBack + perpendicular * 6.0f;

    const glm::vec2 arrowRight =
        arrowBack - perpendicular * 6.0f;

    ui.drawLine(
        end.x, end.y,
        arrowLeft.x, arrowLeft.y,
        3.0f,
        1.0f, 0.65f, 0.10f);

    ui.drawLine(
        end.x, end.y,
        arrowRight.x, arrowRight.y,
        3.0f,
        1.0f, 0.65f, 0.10f);

    ui.drawText(
        "E",
        end.x + 8.0f,
        end.y - 8.0f,
        0.6f,
        1.0f, 0.65f, 0.10f);
}

void Application::renderUI()
{
    // Inicia a interface gráfica.
    ui.begin(window);

    // Reúne os dados que o HUD exibe.
    EnvironmentInfo info;
    info.editing = editingMode;
    switch (activeTransformTool)
    {
    case TransformTool::Move:
        info.tool = "Mover";
        break;
    case TransformTool::Rotate:
        info.tool = "Rotacionar";
        break;
    case TransformTool::Scale:
        info.tool = "Escalar";
        break;
    case TransformTool::Extrude:
        info.tool = "Extrusão";
        break;
    }
    info.viewedModel = selectedModelName;
    info.cameraPosition = camera.position;

    switch (settings.lightType)
    {
    case LightType::Point:
        info.lightType = "Ponto";
        break;
    case LightType::Directional:
        info.lightType = "Direcional";
        break;
    case LightType::Spot:
        info.lightType = "Holofote";
        break;
    }
    info.lightPosition = light.position;
    info.lightColor = glm::vec3(light.color);
    info.lightSelected = lightSelected;

    for (const Scene::Object &object : scene.getObjects())
    {
        ++info.objectCount;
        if (!object.visible)
            continue;

        ++info.visibleObjectCount;
        if (object.model)
            info.faceCount += object.model->getFaceCount();
        else if (object.mesh)
            info.faceCount += object.mesh->getFaces().size();
    }

    const Scene::Object *selectedObject =
        lightSelected ? nullptr : scene.getSelectedObject();
    if (selectedObject)
    {
        info.hasSelection = true;
        info.selectedName = selectedObject->name;
        info.selectedPosition = selectedObject->transform.position;
        info.selectedRotation = selectedObject->transform.rotation;
        info.selectedScale = selectedObject->transform.scale;

        const Scene::FaceSelection &face = scene.getSelectedFace();
        info.hasFace = face.valid;
        info.selectedFace = face.faceIndex;
    }

    // Desenha o HUD e as informações da cena.
    hud.draw(ui, info);

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
        {
            Scene::Object *selected = scene.getSelectedObject();

            if (selected && !lightSelected)
            {
                ScaleGizmoProjection gizmo;

                if (buildScaleGizmo(
                        window,
                        camera,
                        settings.fov,
                        *selected,
                        gizmo))
                {
                    ui.drawRect(
                        gizmo.center.x - 4.0f,
                        gizmo.center.y - 4.0f,
                        8.0f,
                        8.0f,
                        0.90f, 0.90f, 0.90f);

                    for (const ScaleHandleProjection &handle : gizmo.handles)
                    {
                        if (!handle.visible)
                            continue;

                        const float r = handle.color.r;
                        const float g = handle.color.g;
                        const float b = handle.color.b;

                        ui.drawLine(
                            gizmo.center.x,
                            gizmo.center.y,
                            handle.screenPosition.x,
                            handle.screenPosition.y,
                            3.0f,
                            r, g, b);

                        ui.drawRect(
                            handle.screenPosition.x - 6.0f,
                            handle.screenPosition.y - 6.0f,
                            12.0f,
                            12.0f,
                            r, g, b);

                        ui.drawText(
                            handle.label,
                            handle.screenPosition.x + 9.0f,
                            handle.screenPosition.y - 9.0f,
                            0.55f,
                            r, g, b);
                    }
                }
            }

            break;
        }
        case TransformTool::Extrude:
            drawSelectedFaceOutline();
            drawExtrudeGizmo();
            break;
        }

        if (lightSelected)
            lightPanel.draw(window, ui, settings);

        // Desenha a barra de ferramentas.
        drawTransformToolbar();
    }

    // Finaliza a interface gráfica.
    ui.end();
}