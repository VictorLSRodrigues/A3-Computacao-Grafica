#include "physics_world.h"



void PhysicsWorld::setGravity(const glm::vec3& gravity)
{
    gravity_ = gravity;
}


const glm::vec3& PhysicsWorld::gravity() const
{
    return gravity_;
}


void PhysicsWorld::addBody(Transform& transform, const glm::vec3& initialVelocity)
{
    bodies_.push_back({&transform, initialVelocity});
}


void PhysicsWorld::update(float deltaTime)
{
    if (deltaTime <= 0.0f)
        return;

    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        // Euler semi-implicito: atualiza velocidade antes da posicao.
        body.velocity += gravity_ * deltaTime;
        body.transform->translate(body.velocity * deltaTime);
    }
}


void PhysicsWorld::clear()
{
    bodies_.clear();
}


std::size_t PhysicsWorld::bodyCount() const
{
    return bodies_.size();
}
