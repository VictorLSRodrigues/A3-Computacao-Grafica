#pragma once

#include "scene.h"

// Monta os objetos da cena: a pirâmide animada e os modelos carregados de arquivo.
// Retorna false se algum recurso obrigatório não puder ser carregado.
bool buildWorld(Scene& scene);