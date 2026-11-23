#include "engine/editor/editor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

#include "engine/editor/gizmo.h"
#include "engine/scene/picking.h"

Editor::Editor(Window &window, Camera &camera, Scene &scene, Light &light,
               UI &ui)
    : window(window), camera(camera), scene(scene), light(light),
      ui(ui)
{
}

bool Editor::create()
{
    return moveIcon.load(ASSETS_DIR "editor/icons/move.png", false, TextureColorSpace::Linear) &&
           rotateIcon.load(ASSETS_DIR "editor/icons/rotate-3d.png", false, TextureColorSpace::Linear) &&
           scaleIcon.load(ASSETS_DIR "editor/icons/scaling.png", false, TextureColorSpace::Linear) &&
           extrudeIcon.load(ASSETS_DIR "editor/icons/extrude.png", false, TextureColorSpace::Linear);
}

void Editor::handleModeToggle()
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
}

void Editor::update(bool inputEnabled)
{
    // Um objeto que ficou oculto (por exemplo, pelos atalhos da aplicação) deixa de ser selecionado.
    if (selectedObject && !selectedObject->visible)
    {
        selectedObject = nullptr;
        selectedFace.valid = false;
    }

    const bool controlPressed =
        window.keyDown(GLFW_KEY_LEFT_CONTROL) ||
        window.keyDown(GLFW_KEY_RIGHT_CONTROL);
    if (inputEnabled && controlPressed &&
        window.keyPressed(GLFW_KEY_Z) &&
        !draggingGizmo &&
        !draggingRotationGizmo &&
        !draggingScaleGizmo &&
        !draggingExtrusionGizmo)
    {
        undoLastTransform();
    }

    // Seleção e movimentação de objetos no modo de edição.
    const bool editingAvailable = editingMode && inputEnabled;

    if (!editingAvailable)
    {
        if (draggingGizmo || draggingRotationGizmo || draggingScaleGizmo)
            finishTransformDrag();

        draggingGizmo = false;
        draggedAxis = -1;

        draggingRotationGizmo = false;
        rotatingAxis = -1;

        draggingScaleGizmo = false;
        scalingAxis = -1;

        if (draggingExtrusionGizmo)
        {
            scene.cancelFaceExtrusion(selectedObject, selectedFace);
            restorePendingExtrusion();

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
                if (selectedFace.valid)
                {
                    ExtrudeGizmoProjection gizmo;

                    if (buildExtrudeGizmoProjection(
                            window,
                            camera,
                            scene,
                            selectedObject,
                            selectedFace,
                            gizmo) &&
                        isMouseOverExtrudeGizmo(mouse, gizmo))
                    {
                        // Captura o estado completo antes de iniciar a extrusao.
                        UndoEntry extrusionUndo;
                        extrusionUndo.object = selectedObject;
                        extrusionUndo.isExtrusion = true;
                        extrusionUndo.previousFace = selectedFace;

                        bool snapshotReady = false;

                        if (selectedObject && selectedObject->model)
                        {
                            extrusionUndo.previousModel =
                                selectedObject->model->clone();

                            snapshotReady = static_cast<bool>(
                                extrusionUndo.previousModel);
                        }
                        else if (selectedObject && selectedObject->mesh)
                        {
                            extrusionUndo.previousMesh =
                                std::make_shared<Mesh>();

                            snapshotReady =
                                extrusionUndo.previousMesh->copyFrom(
                                    *selectedObject->mesh);

                            if (!snapshotReady)
                                extrusionUndo.previousMesh.reset();
                        }

                        if (!snapshotReady)
                        {
                            std::cerr
                                << "Falha ao salvar estado anterior da extrusao."
                                << std::endl;
                        }
                        else if (scene.beginFaceExtrusion(
                                     selectedObject, selectedFace))
                        {
                            pendingExtrusionUndo = extrusionUndo;
                            pendingExtrusionUndoValid = true;

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
                        createMouseRay(window, camera);

                    const bool faceFound =
                        scene.pickFace(camera.position, rayDirection, selectedObject, selectedFace);

                    handledFaceSelection = true;

                    if (faceFound)
                    {
                        lightSelected = false;

                        const Scene::Object *faceObject =
                            selectedObject;

                        const Scene::FaceSelection &face =
                            selectedFace;

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
                lightSelected ? nullptr : selectedObject;

            if (selected || lightSelected)
            {
                switch (activeTransformTool)
                {
                case TransformTool::Move:
                {
                    const glm::vec3 &selectedPosition =
                        lightSelected ? light.position : selected->transform.position();

                    // Primeiro verifica se o clique atingiu uma seta do gizmo.
                    MoveGizmoProjection gizmo;

                    if (buildMoveGizmo(
                            window,
                            camera,
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
                            draggedObject =
                                lightSelected ? nullptr : selected;
                            draggedLight = lightSelected;
                            if (draggedObject)
                                dragStartTransform = draggedObject->transform;
                            else if (draggedLight)
                                dragStartLight = light;

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
                                createMouseRay(window, camera);

                            glm::vec3 intersection;

                            if (intersectRayPlane(
                                    camera.position,
                                    rayDirection,
                                    selected->transform.position(),
                                    selectedRing.worldAxis,
                                    intersection))
                            {
                                // Guarda a direção inicial do mouse no plano do anel.
                                rotationStartVector = glm::normalize(
                                    intersection - selected->transform.position());

                                // Guarda a rotação original do objeto.
                                rotationStart = selected->transform.rotation();
                                draggedObject = selected;
                                draggedLight = false;
                                dragStartTransform = selected->transform;

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
                                scaleStart = selected->transform.scale();
                                draggedObject = selected;
                                draggedLight = false;
                                dragStartTransform = selected->transform;

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
                    createMouseRay(window, camera);

                selected = scene.pickObject(
                    camera.position,
                    rayDirection);

                // Selecionar um objeto descarta a face selecionada.
                selectedObject = selected;
                selectedFace.valid = false;

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
                lightSelected ? nullptr : selectedObject;

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
                    light.position = newPosition;
                }
                else
                {
                    selected->transform.move(newPosition);
                }
            }
            else
            {
                finishTransformDrag();
                draggingGizmo = false;
                draggedAxis = -1;
            }
        }
        else
        {
            // Soltar o botão encerra a movimentação.
            finishTransformDrag();
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
            Scene::Object *selected = selectedObject;

            if (selected && rotatingAxis >= 0)
            {
                const glm::vec3 rayDirection =
                    createMouseRay(window, camera);

                glm::vec3 intersection;

                if (intersectRayPlane(
                        camera.position,
                        rayDirection,
                        selected->transform.position(),
                        rotatingWorldAxis,
                        intersection))
                {
                    glm::vec3 currentVector =
                        intersection - selected->transform.position();

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
                        glm::vec3 newRotation = selected->transform.rotation();

                        newRotation[rotatingAxis] =
                            rotationStart[rotatingAxis] +
                            rotationAccumulatedAngle;

                        selected->transform.setRotation(newRotation);

                        // Atualiza o vetor para o próximo quadro.
                        rotationStartVector = currentVector;
                    }
                }
            }
            else
            {
                finishTransformDrag();
                draggingRotationGizmo = false;
                rotatingAxis = -1;
            }
        }
        else
        {
            // Soltar o botão encerra a rotação.
            finishTransformDrag();
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
            Scene::Object *selected = selectedObject;

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
                glm::vec3 newScale = selected->transform.scale();

                newScale[scalingAxis] =
                    scaleStart[scalingAxis] * scaleMultiplier;

                selected->transform.setScale(newScale);
            }
            else
            {
                finishTransformDrag();
                draggingScaleGizmo = false;
                scalingAxis = -1;
            }
        }
        else
        {
            // Soltar o botão encerra a escala.
            finishTransformDrag();
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
                if (scene.updateFaceExtrusion(selectedObject, selectedFace, distance))
                    extrusionChanged = true;
            }
        }
        else
        {
            // Ao soltar o mouse, confirma ou cancela a operação.
            if (extrusionChanged)
            {
                scene.finishFaceExtrusion(selectedObject, selectedFace);

                // Registra no Undo somente depois de confirmar a extrusao.
                if (pendingExtrusionUndoValid)
                {
                    undoHistory.push_back(pendingExtrusionUndo);
                    pendingExtrusionUndo = UndoEntry{};
                    pendingExtrusionUndoValid = false;
                }

                std::cout
                    << "[EXTRUDE] Extrusao confirmada: "
                    << extrusionDistance
                    << std::endl;
            }
            else
            {
                scene.cancelFaceExtrusion(selectedObject, selectedFace);
                restorePendingExtrusion();

                std::cout
                    << "[EXTRUDE] Extrusao cancelada."
                    << std::endl;
            }

            draggingExtrusionGizmo = false;
            extrusionDistance = 0.0f;
            extrusionChanged = false;
        }
    }
}

void Editor::recordLightChange(const Light &previousState)
{
    const bool changed =
        glm::any(glm::notEqual(previousState.position, light.position)) ||
        glm::any(glm::notEqual(previousState.target, light.target));
    if (!changed)
        return;

    UndoEntry entry;
    entry.isLight = true;
    entry.lightState = previousState;
    undoHistory.push_back(entry);
}

void Editor::finishTransformDrag()
{
    if (draggedObject &&
        (glm::any(glm::notEqual(
             draggedObject->transform.position(),
             dragStartTransform.position())) ||
         glm::any(glm::notEqual(
             draggedObject->transform.rotation(),
             dragStartTransform.rotation())) ||
         glm::any(glm::notEqual(
             draggedObject->transform.scale(),
             dragStartTransform.scale()))))
    {
        UndoEntry entry;
        entry.object = draggedObject;
        entry.transform = dragStartTransform;
        undoHistory.push_back(entry);
    }
    else if (draggedLight)
    {
        recordLightChange(dragStartLight);
    }

    draggedObject = nullptr;
    draggedLight = false;
}

void Editor::undoLastTransform()
{
    if (undoHistory.empty())
        return;

    const UndoEntry entry = undoHistory.back();
    undoHistory.pop_back();

    if (entry.isLight)
    {
        light = entry.lightState;
    }
    else if (entry.isExtrusion && entry.object)
    {
        // Restaura os recursos anteriores a extrusao.
        entry.object->model = entry.previousModel;
        entry.object->mesh = entry.previousMesh;

        if (selectedObject == entry.object)
            selectedFace = entry.previousFace;
    }
    else if (entry.object)
    {
        // Mantem o Undo existente de movimentacao, rotacao e escala.
        entry.object->transform = entry.transform;
    }
}

void Editor::restorePendingExtrusion()
{
    if (!pendingExtrusionUndoValid)
        return;

    Scene::Object* object = pendingExtrusionUndo.object;

    if (object)
    {
        object->model = pendingExtrusionUndo.previousModel;
        object->mesh = pendingExtrusionUndo.previousMesh;

        if (selectedObject == object)
            selectedFace = pendingExtrusionUndo.previousFace;
    }

    pendingExtrusionUndo = UndoEntry{};
    pendingExtrusionUndoValid = false;
}

void Editor::resetForSceneDefaults()
{
    // Descarta operações antigas do Ctrl + Z.
    undoHistory.clear();

    pendingExtrusionUndo = UndoEntry{};
    pendingExtrusionUndoValid = false;

    // Limpa a seleção.
    selectedObject = nullptr;
    selectedFace = Scene::FaceSelection{};
    lightSelected = false;

    // Cancela os estados dos gizmos.
    draggingGizmo = false;
    draggedAxis = -1;

    draggingRotationGizmo = false;
    rotatingAxis = -1;

    draggingScaleGizmo = false;
    scalingAxis = -1;

    draggingExtrusionGizmo = false;
    extrusionDistance = 0.0f;
    extrusionChanged = false;

    draggedObject = nullptr;
    draggedLight = false;
}

void Editor::drawGizmos()
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
        drawScaleGizmo();
        break;

    case TransformTool::Extrude:
        drawSelectedFaceOutline();
        drawExtrudeGizmo();
        break;
    }
}

// Desenha as ferramentas de transformação no canto superior direito.
void Editor::drawToolbar()
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
    const float panelPadding = 7.0f;
    const float panelX = std::max(0.0f, x - panelPadding);
    const float panelY = std::max(0.0f, y - panelPadding);
    const float panelWidth = totalWidth + panelPadding * 2.0f;
    const float panelHeight = buttonSize + panelPadding * 2.0f;

    // Fundo discreto que agrupa as ferramentas como uma única toolbar.
    ui.addRegion(panelX, panelY, panelWidth, panelHeight);
    ui.drawRect(panelX, panelY, panelWidth, panelHeight,
                theme::panelBackground.r, theme::panelBackground.g,
                theme::panelBackground.b, theme::panelOpacity);
    ui.drawRect(panelX, panelY, panelWidth, 2.0f,
                theme::accent.r, theme::accent.g, theme::accent.b);
    ui.drawRect(panelX, panelY + panelHeight - 1.0f, panelWidth, 1.0f,
                theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
    ui.drawRect(panelX, panelY, 1.0f, panelHeight,
                theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
    ui.drawRect(panelX + panelWidth - 1.0f, panelY, 1.0f, panelHeight,
                theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);

    // Botão Mover.
    if (ui.iconButton(
            moveIcon,
            "Mover",
            x, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Move))
    {
        activeTransformTool = TransformTool::Move;
    }

    // Botão Rotacionar.
    if (ui.iconButton(
            rotateIcon,
            "Rotacionar",
            x + buttonSize + gap, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Rotate))
    {
        activeTransformTool = TransformTool::Rotate;
    }

    // Botão Escalar.
    if (ui.iconButton(
            scaleIcon,
            "Escalar",
            x + (buttonSize + gap) * 2.0f, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Scale))
    {
        activeTransformTool = TransformTool::Scale;
    }

    // Botão Extrudar.
    if (ui.iconButton(
            extrudeIcon,
            "Extrusão",
            x + (buttonSize + gap) * 3.0f, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Extrude))
    {
        activeTransformTool = TransformTool::Extrude;
    }
}

void Editor::drawMoveGizmo()
{
    Scene::Object *selected =
        lightSelected ? nullptr : selectedObject;

    if (!selected && !lightSelected)
        return;

    const glm::vec3 &selectedPosition =
        lightSelected ? light.position : selected->transform.position();

    MoveGizmoProjection gizmo;

    if (!buildMoveGizmo(
            window,
            camera,
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

void Editor::drawRotateGizmo()
{
    if (lightSelected)
        return;

    Scene::Object *selected = selectedObject;

    if (!selected)
        return;

    RotateGizmoProjection gizmo;

    if (!buildRotateGizmo(
            window,
            camera,
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
void Editor::drawSelectedFaceOutline()
{
    const Scene::FaceSelection &selection =
        selectedFace;

    if (!selection.valid)
        return;

    glm::vec3 a, b, c;

    if (!scene.getFacePositions(selectedObject, selectedFace, a, b, c))
        return;

    const int width = window.getWidth();
    const int height = window.getHeight();

    if (width <= 0 || height <= 0)
        return;

    const glm::mat4 viewProjection = camera.viewProjectionMatrix();

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

void Editor::drawExtrudeGizmo()
{
    ExtrudeGizmoProjection gizmo;

    if (!buildExtrudeGizmoProjection(
            window,
            camera,
            scene,
            selectedObject,
            selectedFace,
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

void Editor::drawScaleGizmo()
{
    Scene::Object *selected = selectedObject;

    if (selected && !lightSelected)
    {
        ScaleGizmoProjection gizmo;

        if (buildScaleGizmo(
                window,
                camera,
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
}
