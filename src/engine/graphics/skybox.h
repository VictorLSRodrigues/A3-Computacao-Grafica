#pragma once

#include <string>

#include "camera.h"
#include "cubemap.h"
#include "mesh.h"
#include "shader.h"

// Céu desenhado ao redor da cena: um cubo que acompanha a câmera e mostra um cubemap.
class Skybox
{
public:
    // Carrega as imagens right, left, top, bottom, front e back da pasta indicada
    // (que deve terminar com "/") e o shader do céu. Exige a janela criada.
    bool load(const std::string& folder, const std::string& extension = "png");

    // Desenha o céu; deve ser chamado depois dos objetos da cena. Não faz nada se não foi carregado.
    void draw(Camera& camera, float fovDegrees, float nearPlane, float farPlane);

private:
    Cubemap cubemap;
    Shader shader;
    Mesh cube;
    bool loaded = false;
};