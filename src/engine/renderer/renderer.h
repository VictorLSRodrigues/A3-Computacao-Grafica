#pragma once

#include "engine/graphics/camera/camera.h"
#include "engine/graphics/resources/shader.h"
#include "engine/graphics/environment/skybox.h"
#include "engine/graphics/resources/texture.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"

// Desenha a cena 3D: guarda os shaders e a textura padrão, percorre os objetos da Scene
// e define como um quadro é montado.
class Renderer
{
public:
    // Mostra o céu ao redor da cena.
    bool showSkybox = kDefaultShowSkybox;

    // Controla separadamente a contribuição difusa e as reflexões do cubemap.
    float ambientStrength = kDefaultAmbientStrength;
    float environmentReflectionStrength = kDefaultEnvironmentReflectionStrength;

    // Carrega shaders e textura padrão e liga os testes de profundidade e de face.
    // Exige a janela criada, pois usa OpenGL. Retorna false se algo falhar.
    bool init();

    // Desenha um quadro: a cena iluminada, o marcador da luz e o céu.
    // A Scene só é consultada; quem a desenha é o Renderer.
    void render(const Scene &scene, const Light &light, Camera &camera);

    // Volta as opções de renderização aos valores padrão.
    void resetSettings();

private:
    static constexpr bool kDefaultShowSkybox = true;
    static constexpr float kDefaultAmbientStrength = 0.12f;
    static constexpr float kDefaultEnvironmentReflectionStrength = 0.35f;

    bool loadShaders();

    // Desenha os objetos visíveis com o shader da cena, na ordem em que estão na Scene.
    void drawScene(const Scene &scene);

    Shader sceneShader;
    Shader lightShader;
    Texture defaultTexture;
    Skybox skybox;
};