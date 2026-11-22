#include "renderer.h"

#include <iostream>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/graphics/geometry/shapes.h"

namespace
{
    float srgbToLinear(float value)
    {
        return value <= 0.04045f
            ? value / 12.92f
            : std::pow((value + 0.055f) / 1.055f, 2.4f);
    }
}

bool Renderer::init()
{
    if (!defaultTexture.load(ASSETS_DIR "engine/textures/default.png"))
    {
        std::cout << "Falha ao carregar a textura padrao!" << std::endl;
        return false;
    }

    if (!loadShaders())
        return false;

    if (!lightMarker.createIndexed(
            shapes::cube(),
            shapes::cubeIndices(),
            3,
            GL_TRIANGLES,
            {3}))
    {
        std::cout << "Falha ao criar a malha do marcador da luz." << std::endl;
        return false;
    }

    if (!rainScenario.init())
        return false;

    // O céu é opcional: sem ele, a cena usa a cor de fundo lisa.
    if (!skybox.load(ASSETS_DIR "engine/skybox/"))
        std::cout << "Ceu desativado: nao consegui carregar a pasta assets/skybox." << std::endl;

    // Objetos próximos escondem os que estão atrás; faces voltadas para longe da câmera são descartadas.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

bool Renderer::loadShaders()
{
    if (!sceneShader.createFromFiles(
            ASSETS_DIR "engine/shaders/default.vert",
            ASSETS_DIR "engine/shaders/default.frag"))
    {
        std::cout << "Falha ao criar o shader da cena!" << std::endl;
        return false;
    }

    sceneShader.use();
    sceneShader.setInt("tex0", 0);
    sceneShader.setInt("specular0", 1);

    if (!lightShader.createFromFiles(
            ASSETS_DIR "engine/shaders/light.vert",
            ASSETS_DIR "engine/shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return false;
    }

    return true;
}

void Renderer::resetSettings()
{
    showSkybox = kDefaultShowSkybox;
    showLightMarker = kDefaultShowLightMarker;
    ambientStrength = kDefaultAmbientStrength;
    environmentReflectionStrength = kDefaultEnvironmentReflectionStrength;
    rainScenario.enabled = false;
}

void Renderer::applyLight(const Light &light, const Camera &camera)
{
    sceneShader.setVec4("lightColor", light.color.x, light.color.y, light.color.z, 1.0f);
    sceneShader.setFloat("lightIntensity", light.intensity);
    sceneShader.setVec3("lightPos", light.position.x, light.position.y, light.position.z);
    sceneShader.setVec3("camPos", camera.position.x, camera.position.y, camera.position.z);

    const glm::vec3 direction = light.direction();
    sceneShader.setVec3("lightDir", direction.x, direction.y, direction.z);
    sceneShader.setInt("lightType", static_cast<int>(light.type));
}

void Renderer::drawLightMarker(const Light &light, Camera &camera)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, light.position);
    model = glm::scale(model, glm::vec3(0.2f));

    lightShader.use();
    camera.setViewProjection(lightShader, "camMatrix");
    lightShader.setVec4("lightColor", light.color.x, light.color.y, light.color.z, 1.0f);
    lightShader.setMat4("model", model);

    lightMarker.draw();
}

void Renderer::render(const Scene &scene, const Light &light, Camera &camera)
{
    // As cores do fundo são definidas em sRGB, mas glClearColor recebe
    // componentes lineares quando o framebuffer sRGB está habilitado.
    if (rainScenario.enabled)
        glClearColor(srgbToLinear(0.035f), srgbToLinear(0.055f), srgbToLinear(0.075f), 1.0f);
    else
        glClearColor(srgbToLinear(0.1f), srgbToLinear(0.2f), srgbToLinear(0.3f), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A cena, iluminada pela luz escolhida.
    sceneShader.use();
    sceneShader.setFloat("ambientStrength", ambientStrength);
    sceneShader.setFloat("environmentReflectionStrength", environmentReflectionStrength);

    applyLight(light, camera);
    camera.setViewProjection(sceneShader, "camMatrix");

    // Ambiente para reflexões dos materiais.
    sceneShader.setInt("environmentMap", 5);
    sceneShader.setInt("hasEnvironmentMap",
                       skybox.bindEnvironment(5) ? 1 : 0);

    drawScene(scene);

    // O cubo que marca onde a luz está é opcional.
    if (showLightMarker)
        drawLightMarker(light, camera);

    // O céu vai por último: só ocupa o que nenhum objeto cobriu.
    if (showSkybox)
        skybox.draw(camera);

    rainScenario.render(camera);
}

void Renderer::drawScene(const Scene &scene)
{
    for (const Scene::Object &object : scene.getObjects())
    {
        // Não desenha objetos ocultos.
        if (!object.visible)
            continue;

        const glm::mat4 matrix = object.transform.matrix();

        if (object.model)
        {
            // O modelo envia a matriz de cada parte dele,
            // já combinada com a transformação do objeto.
            object.model->draw(sceneShader, defaultTexture, matrix);
        }
        else if (object.mesh)
        {
            sceneShader.setMat4("model", matrix);
            object.mesh->draw(sceneShader, &defaultTexture);
        }
    }
}
