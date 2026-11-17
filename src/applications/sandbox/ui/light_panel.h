#pragma once

#include "engine/ui/UI.h"
#include "engine/core/settings.h"
#include "engine/platform/window.h"

class LightPanel
{
public:
    void draw(const Window &window, UI &ui, Settings &settings);
};
