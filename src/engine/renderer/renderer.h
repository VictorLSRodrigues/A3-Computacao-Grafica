#pragma once

#include "engine/graphics/camera/camera.h"
#include "engine/graphics/resources/shader.h"
#include "engine/graphics/environment/skybox.h"
#include "engine/graphics/resources/texture.h"
#include "engine/scene/light.h"
#include "engine/scene/scene.h"
#include "engine/core/settings.h"

// Desenha a cena 3D: guarda os shaders e a textura padrão, percorre os objetos da Scene
// e define como um quadro é montado.
class Renderer
{
public:
    // Carrega shaders e textura padrão e liga os testes de profundidade e de face.
    // Exige a janela criada, pois usa OpenGL. Retorna false se algo falhar.
    bool init();

    // Desenha um quadro: a cena iluminada, o marcador da luz e o céu.
    // A Scene só é consultada; quem a desenha é o Renderer.
    void render(const Scene &scene, const Light &light, Camera &camera, const Settings &settings);

private:
    bool loadShaders();

    // Desenha os objetos visíveis com o shader da cena, na ordem em que estão na Scene.
    void drawScene(const Scene &scene);

    Shader sceneShader;
    Shader lightShader;
    Texture defaultTexture;
    Skybox skybox;
};