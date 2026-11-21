#pragma once

#include <functional>

#include "engine/ui/UI.h"
#include "engine/renderer/renderer.h"
#include "engine/graphics/camera/camera.h"
#include "engine/scene/light.h"
#include "engine/platform/window.h"

// Controla as páginas e ações do menu; a classe UI desenha seus elementos.
class Menu
{
private:
    enum class Page { Main, Options };
    enum class OptionsCategory { Camera, Environment, Lighting, Doom };

    bool open = false;
    Page page = Page::Main;
    OptionsCategory optionsCategory = OptionsCategory::Camera;

    void drawMain(const Window& window, UI& ui);
    void drawOptions(const Window& window, UI& ui, Renderer& renderer, Camera& camera, Light& light,
                     const std::function<void()>& launchDoom);

public:
    void update(const Window& window); // ESC abre, fecha ou retorna à página anterior.
    void draw(const Window& window, UI& ui, Renderer& renderer, Camera& camera, Light& light,
              const std::function<void()>& launchDoom);
    bool isOpen() const { return open; }
};