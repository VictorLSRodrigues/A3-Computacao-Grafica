#pragma once

#include "UI.h"
#include "settings.h"
#include "window.h"

// Controla as páginas e ações do menu; a classe UI desenha seus elementos.
class Menu
{
private:
    enum class Page { Main, Options, Light };
    bool open = false;
    Page page = Page::Main;

    void drawMain(const Window& window, UI& ui);
    void drawOptions(const Window& window, UI& ui, Settings& settings);
    void drawLight(const Window& window, UI& ui, Settings& settings);

public:
    void update(const Window& window); // ESC abre, fecha ou retorna à página anterior.
    void draw(const Window& window, UI& ui, Settings& settings);
    bool isOpen() const { return open; }
};