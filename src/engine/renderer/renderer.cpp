#include "renderer.h"

#include <array>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/graphics/geometry/shapes.h"

namespace
{
using FrustumPlanes = std::array<glm::vec4, 6>;

bool finite(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}

bool extractFrustumPlanes(
    const glm::mat4& viewProjection,
    FrustumPlanes& planes)
{
    glm::vec4 rows[4];
    for (int row = 0; row < 4; ++row)
    {
        rows[row] = glm::vec4(
            viewProjection[0][row],
            viewProjection[1][row],
            viewProjection[2][row],
            viewProjection[3][row]);
    }

    planes = {{
        rows[3] + rows[0], rows[3] - rows[0],
        rows[3] + rows[1], rows[3] - rows[1],
        rows[3] + rows[2], rows[3] - rows[2]}};

    for (glm::vec4& plane : planes)
    {
        const float normalLength = glm::length(glm::vec3(plane));
        if (!std::isfinite(normalLength) || normalLength <= 1e-7f ||
            !std::isfinite(plane.w))
            return false;
        plane /= normalLength;
    }
    return true;
}

bool isInsideFrustum(
    const BoundingBox& bounds,
    const glm::mat4& modelMatrix,
    const FrustumPlanes& planes)
{
    if (!bounds.valid || !finite(bounds.min) || !finite(bounds.max))
        return true;

    const glm::vec3 localCenter = (bounds.min + bounds.max) * 0.5f;
    const glm::vec3 localHalfExtents =
        glm::max((bounds.max - bounds.min) * 0.5f, glm::vec3(0.0f));
    const glm::vec3 center(
        modelMatrix * glm::vec4(localCenter, 1.0f));
    const glm::mat3 basis(modelMatrix);
    const glm::vec3 axisX = basis[0];
    const glm::vec3 axisY = basis[1];
    const glm::vec3 axisZ = basis[2];
    if (!finite(center) || !finite(axisX) ||
        !finite(axisY) || !finite(axisZ) || !finite(localHalfExtents))
        return true;

    for (const glm::vec4& plane : planes)
    {
        const glm::vec3 normal(plane);
        const float projectedRadius =
            std::fabs(glm::dot(normal, axisX)) * localHalfExtents.x +
            std::fabs(glm::dot(normal, axisY)) * localHalfExtents.y +
            std::fabs(glm::dot(normal, axisZ)) * localHalfExtents.z;
        const float distance = glm::dot(normal, center) + plane.w;
        const float margin = 1e-4f *
            (1.0f + glm::length(center) + projectedRadius);
        if (distance + projectedRadius < -margin)
            return false;
    }
    return true;
}

void setFrontFaceForTransform(const glm::mat4& matrix)
{
    const float determinant = glm::determinant(glm::mat3(matrix));
    glFrontFace(std::isfinite(determinant) && determinant < 0.0f
        ? GL_CW : GL_CCW);
}
} // namespace

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

    rainScenario.init();

    if (!rainRenderer.init())
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
    lightShader.setInt("uFramebufferSRGB",
                       glIsEnabled(GL_FRAMEBUFFER_SRGB) == GL_TRUE ? 1 : 0);
    camera.setViewProjection(lightShader, "camMatrix");
    lightShader.setVec4("lightColor", light.color.x, light.color.y, light.color.z, 1.0f);
    lightShader.setMat4("model", model);

    lightMarker.draw();
}

void Renderer::render(const Scene &scene, const Light &light, Camera &camera)
{
    // glClear não passa pela conversão automática de GL_FRAMEBUFFER_SRGB.
    // As cores de fundo já estão definidas nos valores sRGB desejados.
    if (rainScenario.enabled)
        glClearColor(0.035f, 0.055f, 0.075f, 1.0f);
    else
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A cena, iluminada pela luz escolhida.
    const bool framebufferSRGB = glIsEnabled(GL_FRAMEBUFFER_SRGB) == GL_TRUE;

    sceneShader.use();
    sceneShader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);
    sceneShader.setFloat("ambientStrength", ambientStrength);
    sceneShader.setFloat("environmentReflectionStrength", environmentReflectionStrength);

    applyLight(light, camera);
    camera.setViewProjection(sceneShader, "camMatrix");

    // Ambiente para reflexões dos materiais.
    sceneShader.setInt("environmentMap", 5);
    sceneShader.setInt("hasEnvironmentMap",
                       skybox.bindEnvironment(5) ? 1 : 0);

    drawScene(scene, camera.viewProjectionMatrix());

    // O cubo que marca onde a luz está é opcional.
    if (showLightMarker)
        drawLightMarker(light, camera);

    // O céu vai por último: só ocupa o que nenhum objeto cobriu.
    if (showSkybox)
        skybox.draw(camera, framebufferSRGB);

    rainRenderer.render(rainScenario, camera, framebufferSRGB);
}

void Renderer::drawScene(
    const Scene &scene,
    const glm::mat4 &viewProjection)
{
    FrustumPlanes frustumPlanes{};
    const bool canCull = extractFrustumPlanes(viewProjection, frustumPlanes);

    for (const Scene::Object &object : scene.getObjects())
    {
        // Não desenha objetos ocultos.
        if (!object.visible)
            continue;

        const glm::mat4 matrix = object.transform.matrix();
        const BoundingBox* bounds = object.boundingBox();
        if (canCull && bounds &&
            !isInsideFrustum(*bounds, matrix, frustumPlanes))
            continue;

        if (object.model)
        {
            // O modelo envia a matriz de cada parte dele,
            // já combinada com a transformação do objeto.
            object.model->draw(sceneShader, defaultTexture, matrix);
        }
        else if (object.mesh)
        {
            setFrontFaceForTransform(matrix);
            sceneShader.setMat4("model", matrix);
            object.mesh->draw(sceneShader, &defaultTexture);
        }
        glFrontFace(GL_CCW);
    }
}
