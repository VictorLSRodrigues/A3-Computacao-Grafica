#pragma once

#include "engine/graphics/buffers/VAO.h"
#include "engine/graphics/buffers/VBO.h"
#include "engine/graphics/resources/shader.h"
#include "engine/scene/scenarios/rain_scenario.h"

class Camera;

// Desenha a chuva de um RainScenario: guarda o shader e os buffers do OpenGL
// e envia a cada quadro os segmentos calculados pelo cenário.
class RainRenderer
{
public:
    // Carrega o shader da chuva. Exige a janela criada, pois usa OpenGL.
    bool init();

    // Desenha a chuva se o cenário estiver ativo.
    void render(const RainScenario &rain, Camera &camera, bool framebufferSRGB);

private:
    VAO vao;
    VBO vbo;
    Shader shader;
};
