#pragma once

#include "UI.h"
#include "settings.h"
#include "window.h"

// O menu: decide o que aparece e o que cada botão faz. Quem desenha é a UI.
class Menu
{
private:
    enum class Page { Main, Options };

    bool aberto = false;
    Page page = Page::Main;

    void drawMain(const Window& window, UI& ui);
    void drawOptions(const Window& window, UI& ui, Settings& settings);

    // Fundo escurecido, painel centralizado e título. Devolve o canto superior esquerdo do painel.
    void drawPanel(const Window& window, UI& ui, const char* title,
                   float panelW, float panelH, float& px, float& py);

public:
    void atualizar(const Window& janela);                       // ESC abre, fecha e volta
    void desenhar(const Window& janela, UI& ui, Settings& settings);
    bool estaAberto() const { return aberto; }
};