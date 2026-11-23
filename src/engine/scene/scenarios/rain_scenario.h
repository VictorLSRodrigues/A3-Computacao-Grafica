#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

// Cenário de chuva: guarda as gotas e calcula onde cada uma está em um instante.
// Não desenha nada nem usa OpenGL; quem o desenha é a RainRenderer, do renderer.
class RainScenario
{
public:
    bool enabled = false;

    // Gera as posições iniciais das gotas. O resultado é sempre o mesmo, para a chuva ser reproduzível.
    void init();

    // Calcula os segmentos de reta que representam as gotas no instante informado, em segundos.
    // Escreve 6 valores por gota: x, y, z do começo e x, y, z do fim.
    void buildSegments(double timeSeconds, std::vector<float> &vertices) const;

private:
    std::vector<glm::vec3> drops;
};
