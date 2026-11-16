#pragma once

#include <glm/glm.hpp>

// Cores e medidas compartilhadas pela interface, para que mudar o visual
// seja trocar um valor aqui e não procurar números espalhados.
namespace theme
{
    // Cores em RGB, de 0 a 1.
    inline const glm::vec3 accent{0.25f, 0.55f, 0.95f};          // faixa do topo dos painéis
    inline const glm::vec3 panelBackground{0.08f, 0.10f, 0.15f}; // fundo dos painéis
    inline const glm::vec3 buttonNeutral{0.25f, 0.30f, 0.40f};
    inline const glm::vec3 buttonPrimary{0.20f, 0.45f, 0.85f};
    inline const glm::vec3 buttonDanger{0.70f, 0.15f, 0.15f};
    inline const glm::vec3 text{1.0f, 1.0f, 1.0f};

    // Opacidades.
    constexpr float panelOpacity = 0.96f;
    constexpr float dimOpacity = 0.55f; // escurecimento da tela atrás de um painel modal

    // Medidas dos painéis, em pixels.
    constexpr float accentBarHeight = 4.0f;
    constexpr float titleScale = 0.85f;
}
