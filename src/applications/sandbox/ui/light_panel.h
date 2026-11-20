#pragma once

#include "engine/ui/UI.h"
#include "engine/scene/light.h"
#include "engine/platform/window.h"

class LightPanel
{
public:
    void draw(const Window &window, UI &ui, Light &light);
};
