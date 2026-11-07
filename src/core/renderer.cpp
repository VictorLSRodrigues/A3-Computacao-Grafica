#include "renderer.h"

#include <iostream>

bool Renderer::init()
{
    if (!defaultTexture.load(ASSETS_DIR "textures/default.png"))
    {
        std::cout << "Falha ao carregar a textura padrao!" << std::endl;
        return false;
    }

    if (!loadShaders())
        return false;

    // O céu é opcional: sem ele, a cena usa a cor de fundo lisa.
    if (!skybox.load(ASSETS_DIR "skybox/"))
        std::cout << "Ceu desativado: nao consegui carregar a pasta assets/skybox." << std::endl;

    // Objetos próximos escondem os que estão atrás; faces voltadas para longe da câmera são descartadas.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

bool Renderer::loadShaders()
{
    if (!sceneShader.createFromFiles(
            ASSETS_DIR "shaders/default.vert",
            ASSETS_DIR "shaders/default.frag"))
    {
        std::cout << "Falha ao criar o shader da cena!" << std::endl;
        return false;
    }

    sceneShader.use();
    sceneShader.setInt("tex0", 0);
    sceneShader.setInt("specular0", 1);

    if (!lightShader.createFromFiles(
            ASSETS_DIR "shaders/light.vert",
            ASSETS_DIR "shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return false;
    }

    return true;
}

void Renderer::render(Scene &scene, const Light &light, Camera &camera, const Settings &settings)
{
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A cena, iluminada pela luz escolhida.
    sceneShader.use();
    light.apply(sceneShader, settings.lightType, camera);
    camera.setViewProjection(settings.fov, nearPlane, farPlane, sceneShader, "camMatrix");
    scene.draw(sceneShader, camera, defaultTexture);

    // O cubo que marca onde a luz está.
    lightShader.use();
    camera.setViewProjection(settings.fov, nearPlane, farPlane, lightShader, "camMatrix");
    light.drawMarker(lightShader);
    
    // O céu vai por último: só ocupa o que nenhum objeto cobriu.
    if (settings.showSkybox)
        skybox.draw(camera, settings.fov, nearPlane, farPlane);
}