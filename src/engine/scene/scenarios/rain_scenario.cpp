#include "rain_scenario.h"

#include <cmath>
#include <iostream>
#include <random>

#include <GLFW/glfw3.h>

#include "engine/graphics/camera/camera.h"

bool RainScenario::init()
{
    if (!shader.createFromFiles(
            ASSETS_DIR "engine/shaders/rain.vert",
            ASSETS_DIR "engine/shaders/rain.frag"))
    {
        std::cout << "Falha ao criar o shader da chuva!" << std::endl;
        return false;
    }

    drops.clear();
    drops.reserve(700);

    std::mt19937 generator(12345);
    std::uniform_real_distribution<float> horizontal(-12.0f, 12.0f);
    std::uniform_real_distribution<float> height(-2.0f, 10.0f);
    std::uniform_real_distribution<float> depth(-12.0f, 12.0f);

    for (std::size_t i = 0; i < 700; ++i)
        drops.emplace_back(horizontal(generator), height(generator), depth(generator));

    return true;
}

void RainScenario::render(Camera &camera, bool framebufferSRGB)
{
    if (!enabled)
        return;

    const double time = glfwGetTime();
    std::vector<float> vertices;
    vertices.reserve(drops.size() * 6);

    for (const glm::vec3 &drop : drops)
    {
        const float speed = 7.5f + std::abs(drop.x) * 0.18f;
        double wrappedY = std::fmod(
            static_cast<double>(drop.y) - time * speed + 2.0,
            12.0);
        if (wrappedY < 0.0)
            wrappedY += 12.0;
        const float y = static_cast<float>(wrappedY - 2.0);

        vertices.push_back(drop.x);
        vertices.push_back(y);
        vertices.push_back(drop.z);
        vertices.push_back(drop.x - 0.015f);
        vertices.push_back(y - 0.28f);
        vertices.push_back(drop.z);
    }

    vao.Bind();
    vbo.Upload(vertices, GL_STREAM_DRAW);
    vbo.Bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    vbo.Unbind();

    shader.use();
    shader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);
    camera.setViewProjection(shader, "camMatrix");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size() / 3));
    glDisable(GL_BLEND);

    vao.Unbind();
}
