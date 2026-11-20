#pragma once

#include "engine/ui/UI.h"
#include "engine/core/settings.h"
#include "engine/scene/light.h"
#include "engine/platform/window.h"

// Controla as páginas e ações do menu; a classe UI desenha seus elementos.
class Menu
{
private:
    enum class Page { Main, Options };
    bool open = false;
    Page page = Page::Main;

    void drawMain(const Window& window, UI& ui);
    void drawOptions(const Window& window, UI& ui, Settings& settings, Light& light);

public:
    void update(const Window& window); // ESC abre, fecha ou retorna à página anterior.
    void draw(const Window& window, UI& ui, Settings& settings, Light& light);
    bool isOpen() const { return open; }
};