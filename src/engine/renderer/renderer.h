#pragma once

#include "engine/graphics/camera.h"
#include "engine/graphics/shader.h"
#include "engine/graphics/skybox.h"
#include "engine/graphics/texture.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/core/settings.h"

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

    Shader sceneShader;
    Shader lightShader;
    Texture defaultTexture;
    Skybox skybox;
};