#include "bounding_box.h"

// Expande a caixa delimitadora para incluir um novo vértice.
void BoundingBox::expand(const glm::vec3& point)
{
    // Verifica se este é o primeiro ponto recebido.
    if (!valid)
    {
        // Inicializa os limites com a posição do primeiro vértice.
        min = point;
        max = point;

        // Indica que a Bounding Box já possui limites válidos.
        valid = true;

        return;
    }

    // Verifica se o ponto está além dos limites mínimos.
    if (point.x < min.x) min.x = point.x;
    if (point.y < min.y) min.y = point.y;
    if (point.z < min.z) min.z = point.z;

    // Verifica se o ponto está além dos limites máximos.
    if (point.x > max.x) max.x = point.x;
    if (point.y > max.y) max.y = point.y;
    if (point.z > max.z) max.z = point.z;
}