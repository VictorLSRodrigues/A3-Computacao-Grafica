#include "light.h"

void Light::resetProperties()
{
    type = kDefaultType;
    color = kDefaultColor;
    position = kDefaultPosition;
    target = kDefaultTarget;
    intensity = kDefaultIntensity;
}

glm::vec3 Light::direction() const
{
    const glm::vec3 toTarget = target - position;

    // Se posição e alvo coincidem não há direção; aponta para baixo.
    if (glm::length(toTarget) < 0.0001f)
        return glm::vec3(0.0f, -1.0f, 0.0f);

    return glm::normalize(toTarget);
}
