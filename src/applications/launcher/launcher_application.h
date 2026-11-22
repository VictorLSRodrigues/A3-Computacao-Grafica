#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "engine/application/application.h"
#include "application_discovery.h"
#include "engine/ui/UI.h"

class LauncherApplication : public rotor::Application
{
public:
    LauncherApplication();

protected:
    bool onInitialize() override;
    void onRender() override;

private:
    UI ui;
    std::vector<DiscoveredApplication> applications;
    std::size_t firstVisibleRow = 0;
    std::string errorMessage;

    void draw();
    void drawApplicationCard(const DiscoveredApplication &app,
                             float x, float y, float width, float height);
    bool launch(const DiscoveredApplication &app);
};
