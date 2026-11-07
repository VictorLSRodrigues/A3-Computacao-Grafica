#pragma once

#include "camera.h"
#include "graphics/shader.h"
#include "graphics/skybox.h"
#include "graphics/texture.h"
#include "scene/light.h"
#include "scene/scene.h"
#include "settings.h"

// Desenha a cena 3D: guarda os shaders e a textura padrão e define como um quadro é montado.
class Renderer
{
public:
    // Carrega shaders e textura padrão e liga os testes de profundidade e de face.
    // Exige a janela criada, pois usa OpenGL. Retorna false se algo falhar.
    bool init();

    // Desenha um quadro: a cena iluminada e o marcador da luz.
    void render(Scene &scene, const Light &light, Camera &camera, const Settings &settings);

private:
    bool loadShaders();

    // Distâncias mínima e máxima visíveis pela câmera.
    static constexpr float nearPlane = 0.1f;
    static constexpr float farPlane = 100.0f;

    Shader sceneShader;
    Shader lightShader;
    Texture defaultTexture;
    Skybox skybox;
};