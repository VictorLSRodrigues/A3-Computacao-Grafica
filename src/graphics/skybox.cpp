#include "skybox.h"

#include <iostream>

#include "shapes.h"

bool Skybox::load(const std::string& folder, const std::string& extension)
{
    loaded = false;

    // A ordem segue a do OpenGL: +X, -X, +Y, -Y, +Z, -Z.
    const std::array<std::string, 6> faces = {
        folder + "right." + extension,
        folder + "left." + extension,
        folder + "top." + extension,
        folder + "bottom." + extension,
        folder + "front." + extension,
        folder + "back." + extension};

    if (!cubemap.load(faces))
        return false;

    if (!shader.createFromFiles(
            ASSETS_DIR "shaders/sky.vert",
            ASSETS_DIR "shaders/sky.frag"))
    {
        std::cout << "Falha ao criar o shader do ceu!" << std::endl;
        return false;
    }

    if (!cube.createIndexed(shapes::cube(), shapes::cubeIndices(), 3, GL_TRIANGLES, {3}))
    {
        std::cout << "Falha ao criar o cubo do ceu!" << std::endl;
        return false;
    }

    shader.use();
    shader.setInt("skybox", 0);

    loaded = true;
    return true;
}

void Skybox::draw(Camera& camera, float fovDegrees, float nearPlane, float farPlane)
{
    if (!loaded)
        return;

    // O céu fica na profundidade máxima; aceitar profundidade igual evita cobrir os objetos.
    glDepthFunc(GL_LEQUAL);

    // A câmera está dentro do cubo, então as faces voltadas para dentro precisam aparecer.
    glDisable(GL_CULL_FACE);

    shader.use();
    camera.setViewProjection(fovDegrees, nearPlane, farPlane, shader, "camMatrix", true);
    cubemap.bind(0);
    cube.draw();

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}