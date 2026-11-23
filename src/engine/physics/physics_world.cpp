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
    Body body;
    body.transform = &transform;
    body.velocity = initialVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    bodies_.push_back(body);
}

void PhysicsWorld::update(float frameTime)
{
    if (frameTime <= 0.0f)
        return;

    accumulator_ += frameTime;

    // Consome o tempo acumulado em passos de tamanho fixo.
    int steps = 0;
    while (accumulator_ + kTolerance >= kFixedStep && steps < kMaxStepsPerFrame)
    {
        step(static_cast<float>(kFixedStep));
        accumulator_ -= kFixedStep;
        ++steps;
    }

    // Se o quadro foi lento demais para alcancar o tempo real, descarta o atraso:
    // a simulacao fica em camera lenta em vez de travar.
    if (steps == kMaxStepsPerFrame)
        accumulator_ = 0.0;
}

void PhysicsWorld::step(float dt)
{
    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        // Euler semi-implicito: atualiza a velocidade antes da posicao.
        body.velocity += gravity_ * dt;
        body.transform->translate(body.velocity * dt);
    }

    ++stepCount_;
}

void PhysicsWorld::reset()
{
    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        body.transform->move(body.startPosition);
        body.velocity = body.startVelocity;
    }

    accumulator_ = 0.0;
}

void PhysicsWorld::recordStartPositions()
{
    for (Body& body : bodies_)
    {
        if (body.transform)
            body.startPosition = body.transform->position();
    }
}

void PhysicsWorld::clear()
{
    bodies_.clear();
    accumulator_ = 0.0;
}

std::size_t PhysicsWorld::bodyCount() const
{
    return bodies_.size();
}

unsigned long long PhysicsWorld::stepCount() const
{
    return stepCount_;
}