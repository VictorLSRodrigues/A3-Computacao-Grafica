#include "rain_renderer.h"

#include <iostream>
#include <vector>

#include <GLFW/glfw3.h>

#include "engine/graphics/camera/camera.h"

bool RainRenderer::init()
{
    if (!shader.createFromFiles(
            ASSETS_DIR "engine/shaders/rain.vert",
            ASSETS_DIR "engine/shaders/rain.frag"))
    {
        std::cout << "Falha ao criar o shader da chuva!" << std::endl;
        return false;
    }

    return true;
}

void RainRenderer::render(const RainScenario &rain, Camera &camera, bool framebufferSRGB)
{
    if (!rain.enabled)
        return;

    std::vector<float> vertices;
    rain.buildSegments(glfwGetTime(), vertices);

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
