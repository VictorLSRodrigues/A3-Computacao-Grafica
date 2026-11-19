#pragma once

#include <string>

#include "applications/sandbox/ui/HUD.h"
#include "engine/core/settings.h"
#include "engine/editor/editor.h"
#include "engine/graphics/camera/camera.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"

// Reúne, a partir da cena, da câmera, da luz e do editor, os dados que o HUD exibe.
// Fica separado para o HUD continuar sem depender de Scene, Light nem Camera.
EnvironmentInfo makeEnvironmentInfo(
    const Scene &scene,
    const Camera &camera,
    const Light &light,
    const Settings &settings,
    const Editor &editor,
    const std::string &viewedModel);
