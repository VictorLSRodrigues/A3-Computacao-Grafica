#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "engine/graphics/camera/camera.h"
#include "engine/platform/window.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/graphics/resources/texture.h"
#include "engine/ui/UI.h"
#include <memory>

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

    // Registra alterações da luz feitas diretamente pela interface.
    void recordLightChange(const Light &previousState);

    // Descarta seleção e histórico após restaurar os padrões da cena.
    void resetForSceneDefaults();

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
    Scene::Object *draggedObject = nullptr;
    bool draggedLight = false;
    Transform dragStartTransform;
    Light dragStartLight;

    struct UndoEntry
    {
        Scene::Object *object = nullptr;

        Transform transform;

        bool isLight = false;
        Light lightState;

        // Identifica uma operação de extrusão.
        bool isExtrusion = false;

        // Recursos anteriores à extrusão.
        std::shared_ptr<Model> previousModel;
        std::shared_ptr<Mesh> previousMesh;

        // Face selecionada antes da extrusão.
        Scene::FaceSelection previousFace;
    };

    std::vector<UndoEntry> undoHistory;

    // Estado temporário da extrusão enquanto o usuário arrasta o gizmo.
    UndoEntry pendingExtrusionUndo;
    bool pendingExtrusionUndoValid = false;

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

    // Centro visual fixo durante a operação de escala.
    glm::vec3 scalePivotWorld{0.0f};

    glm::vec3 scaleStart{1.0f};glm::vec3 rotationStartVector{0.0f};

    float scaleFactorPerPixel = 0.0f;

    // Guarda a rotação original do objeto.
    glm::vec3 rotationStart{0.0f};

    // Guarda o vetor inicial do mouse no plano de rotação.
    glm::vec3 rotationPivotWorld{0.0f};

    // Guarda o eixo 3D escolhido.
    glm::vec3 rotatingWorldAxis{0.0f};

    float rotationAccumulatedAngle = 0.0f;

    float dragWorldPerPixel = 0.0f;

    void finishTransformDrag();
    void undoLastTransform();
    void restorePendingExtrusion();

    void drawMoveGizmo();
    void drawRotateGizmo();
    void drawScaleGizmo();
    void drawSelectedFaceOutline();
    void drawExtrudeGizmo();
};
