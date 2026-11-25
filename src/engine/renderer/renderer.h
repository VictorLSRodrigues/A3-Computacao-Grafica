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

    // Mostra o marcador de posição (luzes pontuais) ou o gizmo direcional (sol + seta).
    bool showLightMarker = kDefaultShowLightMarker;

    // Ativa sombras dinâmicas quando a fonte é direcional.
    bool shadowsEnabled = true;

    // Iluminacao de estudio adicional (fill/rim), sem sombras próprias.
    bool studioLightingEnabled = false;
    bool environmentDiffuseEnabled = false;
    bool toneMappingEnabled = true;

    // Oclusao ambiente em espaco de tela; afeta apenas a iluminacao indireta.
    bool ambientOcclusionEnabled = true;
    float ambientOcclusionStrength = 1.10f;

    // Controla separadamente a contribuição difusa e as reflexões do cubemap.
    float ambientStrength = kDefaultAmbientStrength;
    float environmentReflectionStrength = kDefaultEnvironmentReflectionStrength;

    // Carrega shaders e recursos gráficos e liga os testes de profundidade e de face.
    // Exige a janela criada, pois usa OpenGL. Retorna false se algo falhar.
    bool init();
    ~Renderer();

    Renderer() = default;
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

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
    static constexpr float kDefaultAmbientStrength = 0.48f;
    static constexpr float kDefaultEnvironmentReflectionStrength = 0.35f;
    static constexpr float kDefaultAmbientOcclusionStrength = 1.10f;

    bool loadShaders();
    bool createShadowResources();
    bool buildDirectionalLightSpaceMatrix(
        const Scene &scene, const Light &light, glm::mat4 &lightSpaceMatrix) const;
    void renderShadowMap(const Scene &scene, const glm::mat4 &lightSpaceMatrix);
    void drawShadowScene(const Scene &scene);

    bool loadSsaoShaders();
    bool createSsaoResources();
    bool resizeSsaoResources(int width, int height);
    bool renderSsao(const Scene &scene, const Camera &camera,
                    int viewportWidth, int viewportHeight);
    void drawSsaoGeometryScene(const Scene &scene);

    // Envia ao shader da cena as propriedades da fonte de luz.
    void applyLight(const Light &light, const Camera &camera);

    // Desenha o marcador visual das luzes pontuais/spot.
    void drawLightMarker(const Light &light, Camera &camera);

    // Representa uma luz direcional com um sol e uma seta orientados pela direção dos raios.
    void drawDirectionalLightGizmo(const Light &light, Camera &camera);

    // Desenha objetos dentro do frustum, preservando objetos sem bounds válidos.
    void drawScene(const Scene &scene, const glm::mat4 &viewProjection);

    Shader sceneShader;
    Shader lightShader;
    Shader shadowShader;
    Shader ssaoGeometryShader;
    Shader ssaoShader;
    Shader ssaoBlurShader;
    GLuint shadowFramebuffer = 0;
    GLuint shadowDepthTexture = 0;
    bool shadowMapReady = false;
    static constexpr int kShadowMapResolution = 2048;

    GLuint ssaoFramebuffer = 0;
    GLuint ssaoNormalTexture = 0;
    GLuint ssaoDepthTexture = 0;
    GLuint ssaoNoiseTexture = 0;
    GLuint ssaoOutputFramebuffer = 0;
    GLuint ssaoOutputTexture = 0;
    GLuint ssaoBlurFramebuffer = 0;
    GLuint ssaoBlurTexture = 0;
    GLuint fullscreenVao = 0;
    int ssaoWidth = 0;
    int ssaoHeight = 0;
    bool ssaoReady = false;
    static constexpr float kSsaoRadius = 0.45f;
    static constexpr float kSsaoBias = 0.020f;
    Texture defaultTexture;
    Mesh lightMarker;
    Mesh directionalLightGizmo;
    Skybox skybox;
    RainRenderer rainRenderer;
};
