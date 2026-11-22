#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "engine/graphics/buffers/VAO.h"
#include "engine/graphics/buffers/VBO.h"
#include "engine/graphics/resources/shader.h"

class Camera;

class RainScenario
{
public:
    bool enabled = false;

    // Prepara os recursos OpenGL do cenário de chuva.
    bool init();

    // Desenha a chuva se o cenário estiver ativo.
    void render(Camera &camera, bool framebufferSRGB);

private:
    VAO vao;
    VBO vbo;
    Shader shader;
    std::vector<glm::vec3> drops;
};
