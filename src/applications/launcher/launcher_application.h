#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "application_discovery.h"
#include "engine/platform/window.h"
#include "engine/ui/UI.h"

class LauncherApplication
{
public:
    bool init();
    void run();

private:
    Window window;
    UI ui;
    std::filesystem::path executableDirectory;
    std::vector<DiscoveredApplication> applications;
    std::size_t firstVisibleRow = 0;
    std::string errorMessage;

    void draw();
    void drawApplicationCard(const DiscoveredApplication &app,
                             float x, float y, float width, float height);
    bool launch(const DiscoveredApplication &app);
};
