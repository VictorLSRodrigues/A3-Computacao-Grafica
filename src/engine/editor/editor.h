#pragma once

#include <glm/glm.hpp>

#include "engine/graphics/camera/camera.h"
#include "engine/platform/window.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/graphics/resources/texture.h"
#include "engine/ui/UI.h"

// Editor da cena: seleção de objetos, faces e luz, e as ferramentas de transformação
// (mover, rotacionar, escalar e extrudar) com seus gizmos e a barra de ferramentas.
// O Application só cria o editor e chama update() e drawGizmos()/drawToolbar() a cada quadro.
class Editor
{
public:
    enum class TransformTool
    {
        Move,
        Rotate,
        Scale,
        Extrude
    };

    // O editor não é dono de nada: usa os objetos do Application, que precisam
    // viver mais que ele.
    Editor(Window &window, Camera &camera, Scene &scene, Light &light,
           UI &ui);

    Editor(const Editor &) = delete;
    Editor &operator=(const Editor &) = delete;

    // Carrega os recursos visuais próprios da barra de ferramentas.
    bool create();

    // Alterna entre o modo de navegação e o de edição quando TAB é pressionado.
    void handleModeToggle();

    // Trata seleção e arraste dos gizmos. inputEnabled é false quando algo
    // (por exemplo o menu) está com o controle do mouse.
    void update(bool inputEnabled);

    // Desenha o gizmo da ferramenta ativa.
    void drawGizmos();

    // Desenha a barra de ferramentas no canto superior direito.
    void drawToolbar();

    bool isEditing() const { return editingMode; }
    bool isLightSelected() const { return lightSelected; }

    // Objeto e face selecionados (nullptr / face inválida se não há seleção).
    // Quem precisa saber o que está selecionado (HUD, gizmos) consulta o Editor.
    Scene::Object *getSelectedObject() const { return selectedObject; }
    const Scene::FaceSelection &getSelectedFace() const { return selectedFace; }
    TransformTool activeTool() const { return activeTransformTool; }

private:
    Window &window;
    Camera &camera;
    Scene &scene;
    Light &light;
    UI &ui;

    // Ícones pertencem ao Editor, não ao núcleo genérico da UI.
    Texture moveIcon;
    Texture rotateIcon;
    Texture scaleIcon;
    Texture extrudeIcon;

    bool editingMode = false;

    // Estado da seleção. A Scene só responde às consultas de picking.
    Scene::Object *selectedObject = nullptr;
    Scene::FaceSelection selectedFace;
    bool lightSelected = false;

    TransformTool activeTransformTool = TransformTool::Move;

    // Estado do arraste do gizmo de movimentação.
    bool draggingGizmo = false;
    int draggedAxis = -1;

    glm::vec3 dragStartPosition{0.0f};
    glm::vec3 draggedWorldAxis{0.0f};

    glm::vec2 dragStartMouse{0.0f};
    glm::vec2 dragScreenDirection{0.0f};

    // Estado do arraste do gizmo de rotação.
    bool draggingRotationGizmo = false;
    int rotatingAxis = -1;

    // Estado do arraste do gizmo de Escala
    bool draggingScaleGizmo = false;
    int scalingAxis = -1;

    glm::vec2 scaleStartMouse{0.0f};
    glm::vec2 scaleScreenDirection{0.0f};

    // Estado do arraste do gizmo de extrusão.
    bool draggingExtrusionGizmo = false;

    glm::vec2 extrusionStartMouse{0.0f};
    glm::vec2 extrusionScreenDirection{0.0f};

    float extrusionWorldPerPixel = 0.0f;
    float extrusionDistance = 0.0f;

    bool extrusionChanged = false;

    glm::vec3 scalePivotLocal{0.0f};
    glm::vec3 scalePivotWorld{0.0f};

    glm::vec3 scaleStart{1.0f};

    float scaleFactorPerPixel = 0.0f;

    // Guarda a rotação original do objeto.
    glm::vec3 rotationStart{0.0f};

    // Guarda o vetor inicial do mouse no plano de rotação.
    glm::vec3 rotationStartVector{0.0f};

    // Guarda o eixo 3D escolhido.
    glm::vec3 rotatingWorldAxis{0.0f};

    float rotationAccumulatedAngle = 0.0f;

    float dragWorldPerPixel = 0.0f;

    void drawMoveGizmo();
    void drawRotateGizmo();
    void drawScaleGizmo();
    void drawSelectedFaceOutline();
    void drawExtrudeGizmo();
};
