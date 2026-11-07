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

    // Desenha o fundo e o painel; px e py recebem a posição do canto superior esquerdo.
    void drawPanel(const Window& window, UI& ui, const char* title,
                   float panelW, float panelH, float& px, float& py);

public:
    void update(const Window& window); // ESC abre, fecha ou retorna à página anterior.
    void draw(const Window& window, UI& ui, Settings& settings);
    bool isOpen() const { return open; }
};