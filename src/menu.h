#pragma once

#include "UI.h"
#include "window.h"

// O menu: decide o que aparece e o que cada botão faz. Quem desenha é a UI.
class Menu
{
private:
    bool aberto = false;

public:
    void atualizar(const Window& janela);        // ESC abre e fecha
    void desenhar(const Window& janela, UI& ui); // desenha o menu e trata os cliques
    bool estaAberto() const { return aberto; }
};