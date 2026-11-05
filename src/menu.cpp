#include "menu.h"

void Menu::atualizar(const Window& janela)
{
    if (janela.teclaApertou(GLFW_KEY_ESCAPE))
        aberto = !aberto;
}

void Menu::desenhar(const Window& janela, UI& ui)
{
    if (!aberto)
        return;

    const float screenW = (float)janela.getLargura();
    const float screenH = (float)janela.getAltura();

    // Medidas do painel (tudo em pixels). Mexa nestes números pra ajustar o visual.
    const float panelW = 420.0f;
    const float margin = 30.0f;
    const float titleH = 70.0f;
    const float buttonW = panelW - 2.0f * margin;
    const float buttonH = 60.0f;
    const float gap = 16.0f;
    const int count = 3; // quantidade de botões

    const float panelH = margin + titleH + count * buttonH + (count - 1) * gap + margin;
    const float px = (screenW - panelW) / 2.0f;
    const float py = (screenH - panelH) / 2.0f;

    // Fundo escurecido, painel e faixa de destaque
    ui.drawRect(0.0f, 0.0f, screenW, screenH, 0.0f, 0.0f, 0.0f, 0.55f);
    ui.drawRect(px, py, panelW, panelH, 0.08f, 0.10f, 0.15f, 0.96f);
    ui.drawRect(px, py, panelW, 4.0f, 0.25f, 0.55f, 0.95f);

    ui.drawTextCentered("Menu", px, py + margin, panelW, titleH, 0.85f, 1.0f, 1.0f, 1.0f);

    // Botões empilhados
    const float bx = px + margin;
    float by = py + margin + titleH;

    if (ui.button("Continuar", bx, by, buttonW, buttonH, 0.20f, 0.45f, 0.85f))
        aberto = false;
    by += buttonH + gap;

    ui.button("Opções", bx, by, buttonW, buttonH, 0.25f, 0.30f, 0.40f, false);
    by += buttonH + gap;

    if (ui.button("Sair", bx, by, buttonW, buttonH, 0.70f, 0.15f, 0.15f))
        janela.fechar();
}