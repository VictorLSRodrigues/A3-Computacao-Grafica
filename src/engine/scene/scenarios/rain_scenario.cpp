#include "rain_scenario.h"

#include <cmath>
#include <random>

void RainScenario::init()
{
    drops.clear();
    drops.reserve(700);

    std::mt19937 generator(12345);
    std::uniform_real_distribution<float> horizontal(-12.0f, 12.0f);
    std::uniform_real_distribution<float> height(-2.0f, 10.0f);
    std::uniform_real_distribution<float> depth(-12.0f, 12.0f);

    for (std::size_t i = 0; i < 700; ++i)
        drops.emplace_back(horizontal(generator), height(generator), depth(generator));
}

void RainScenario::buildSegments(double timeSeconds, std::vector<float> &vertices) const
{
    vertices.clear();
    vertices.reserve(drops.size() * 6);

    for (const glm::vec3 &drop : drops)
    {
        const float speed = 7.5f + std::abs(drop.x) * 0.18f;
        double wrappedY = std::fmod(
            static_cast<double>(drop.y) - timeSeconds * speed + 2.0,
            12.0);
        if (wrappedY < 0.0)
            wrappedY += 12.0;
        const float y = static_cast<float>(wrappedY - 2.0);

        vertices.push_back(drop.x);
        vertices.push_back(y);
        vertices.push_back(drop.z);
        vertices.push_back(drop.x - 0.015f);
        vertices.push_back(y - 0.28f);
        vertices.push_back(drop.z);
    }
}
