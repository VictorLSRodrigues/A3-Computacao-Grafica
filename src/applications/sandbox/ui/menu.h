#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "engine/ui/UI.h"
#include "engine/renderer/renderer.h"
#include "engine/graphics/camera/camera.h"
#include "engine/scene/light.h"
#include "engine/platform/window.h"

class Menu
{
private:
    enum class Page
    {
        Main,
        Options,
        SaveScene,
        Scenes
    };

    enum class OptionsCategory
    {
        Camera,
        Environment,
        Lighting,
        Window,
        Doom
    };

    bool open = false;
    Page page = Page::Main;

    OptionsCategory optionsCategory = OptionsCategory::Camera;
    bool windowModeChangeFailed = false;

    // Dados da tela de salvamento.
    std::string sceneName;
    std::string saveStatus;
    bool saveSucceeded = false;

    // Paginação da lista de cenas.
    std::size_t scenePage = 0;

    void drawMain(const Window &window, UI &ui);

    void drawSaveScene(
        Window &window,
        UI &ui,
        const std::function<bool(const std::string &)> &saveScene);

    void drawScenes(
        const Window &window,
        UI &ui,
        const std::function<bool(const std::string &)> &loadScene);

    void drawOptions(
        Window &window,
        UI &ui,
        Renderer &renderer,
        Camera &camera,
        Light &light,
        const std::function<void()> &launchDoom,
        const std::function<void()> &restoreDefaults);

public:
    void update(const Window &window);

    void draw(
        Window &window,
        UI &ui,
        Renderer &renderer,
        Camera &camera,
        Light &light,
        const std::function<void()> &launchDoom,
        const std::function<bool(const std::string &)> &saveScene,
        const std::function<bool(const std::string &)> &loadScene,
        const std::function<void()> &restoreDefaults);

    bool isOpen() const
    {
        return open;
    }
};