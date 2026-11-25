#pragma once

#include "engine/graphics/camera/camera.h"
#include "engine/graphics/resources/shader.h"
#include "engine/graphics/environment/skybox.h"
#include "engine/graphics/mesh/mesh.h"
#include "engine/graphics/resources/texture.h"
#include "engine/renderer/rain_renderer.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/scene/scenarios/rain_scenario.h"

// Desenha a cena 3D: guarda os shaders e recursos gráficos e define como um quadro é montado.
class Renderer
{
public:
    // Mostra o céu ao redor da cena.
    bool showSkybox = kDefaultShowSkybox;

    // Mostra o cubo que indica a posição da luz.
    bool showLightMarker = kDefaultShowLightMarker;

    // Controla separadamente a contribuição difusa e as reflexões do cubemap.
    float ambientStrength = kDefaultAmbientStrength;
    float environmentReflectionStrength = kDefaultEnvironmentReflectionStrength;

    // Carrega shaders e recursos gráficos e liga os testes de profundidade e de face.
    // Exige a janela criada, pois usa OpenGL. Retorna false se algo falhar.
    bool init();

    // Desenha um quadro: a cena iluminada, os elementos auxiliares habilitados e o céu.
    // A Scene só é consultada; quem a desenha é o Renderer.
    void render(const Scene &scene, const Light &light, Camera &camera);

    // Volta as opções de renderização aos valores padrão.
    void resetSettings();

    // Estado do cenário de chuva; o desenho fica a cargo do rainRenderer.
    RainScenario rainScenario;

private:
    static constexpr bool kDefaultShowSkybox = true;
    static constexpr bool kDefaultShowLightMarker = true;
    static constexpr float kDefaultAmbientStrength = 0.12f;
    static constexpr float kDefaultEnvironmentReflectionStrength = 0.35f;

    bool loadShaders();

    // Envia ao shader da cena as propriedades da fonte de luz.
    void applyLight(const Light &light, const Camera &camera);

    // Desenha o marcador visual da luz usando sua posição e cor atuais.
    void drawLightMarker(const Light &light, Camera &camera);

    // Desenha objetos dentro do frustum, preservando objetos sem bounds válidos.
    void drawScene(const Scene &scene, const glm::mat4 &viewProjection);

    Shader sceneShader;
    Shader lightShader;
    Texture defaultTexture;
    Mesh lightMarker;
    Skybox skybox;
    RainRenderer rainRenderer;
};
