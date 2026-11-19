#pragma once

#include <vector>

// Representa uma face da geometria.
// Pode conter 3, 4 ou mais vértices.
struct Face
{
    std::vector<unsigned int> vertexIndices;
};