#include "applications/sandbox/ui/environment_info.h"

EnvironmentInfo makeEnvironmentInfo(
    const Scene &scene,
    const Camera &camera,
    const Light &light,
    const Editor &editor,
    const std::string &viewedModel)
{
    EnvironmentInfo info;
    info.editing = editor.isEditing();
    switch (editor.activeTool())
    {
    case Editor::TransformTool::Move:
        info.tool = "Mover";
        break;
    case Editor::TransformTool::Rotate:
        info.tool = "Rotacionar";
        break;
    case Editor::TransformTool::Scale:
        info.tool = "Escalar";
        break;
    case Editor::TransformTool::Extrude:
        info.tool = "Extrusão";
        break;
    }
    info.viewedModel = viewedModel;
    info.cameraPosition = camera.position;

    switch (light.type)
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
    info.lightColor = light.color;
    info.lightSelected = editor.isLightSelected();

    for (const Scene::Object &object : scene.getObjects())
    {
        ++info.objectCount;
        if (!object.visible)
            continue;

        ++info.visibleObjectCount;
        if (object.model)
            info.faceCount += object.model->getFaceCount();
        else if (object.mesh)
            info.faceCount += object.mesh->getGeometry().getFaces().size();
    }

    const Scene::Object *selectedObject =
        editor.isLightSelected() ? nullptr : editor.getSelectedObject();
    if (selectedObject)
    {
        info.hasSelection = true;
        info.selectedName = selectedObject->name;
        info.selectedPosition = selectedObject->transform.position;
        info.selectedRotation = selectedObject->transform.rotation;
        info.selectedScale = selectedObject->transform.scale;

        const Scene::FaceSelection &face = editor.getSelectedFace();
        info.hasFace = face.valid;
        info.selectedFace = face.faceIndex;
    }
    return info;
}
