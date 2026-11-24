#include "physics_world.h"
#include "engine/physics/solver/contact_solver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/gtx/quaternion.hpp>

namespace rotor::physics
{


namespace
{
float clampFriction(float coefficient)
{
    return std::clamp(coefficient, 0.0f, 1.0f);
}

bool isFiniteVector(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}
}

void PhysicsWorld::setGravity(const glm::vec3& gravity)
{
    gravity_ = gravity;
}

const glm::vec3& PhysicsWorld::gravity() const
{
    return gravity_;
}

void PhysicsWorld::setGroundPlane(
    float height,
    float staticFriction,
    float dynamicFriction)
{
    groundHeight_ = height;
    hasGroundPlane_ = true;
    hasGroundBowl_ = false;
    groundStaticFriction_ = clampFriction(staticFriction);
    groundDynamicFriction_ = std::min(
        clampFriction(dynamicFriction), groundStaticFriction_);
}

void PhysicsWorld::setGroundBowl(
    float radius,
    float depth,
    float lipHeight,
    float staticFriction,
    float dynamicFriction)
{
    if (!std::isfinite(radius) || !std::isfinite(depth) ||
        !std::isfinite(lipHeight) || radius <= 0.0f ||
        depth < 0.0f || lipHeight < 0.0f)
    {
        clearGroundPlane();
        return;
    }

    hasGroundPlane_ = false;
    hasGroundBowl_ = true;
    groundBowlRadius_ = radius;
    groundBowlDepth_ = depth;
    groundBowlLipHeight_ = lipHeight;
    groundStaticFriction_ = clampFriction(staticFriction);
    groundDynamicFriction_ = std::min(
        clampFriction(dynamicFriction), groundStaticFriction_);
}

void PhysicsWorld::clearGroundPlane()
{
    hasGroundPlane_ = false;
    hasGroundBowl_ = false;
}

bool PhysicsWorld::addSphereBody(
    Transform& transform,
    const glm::vec3& initialVelocity,
    float sphereRadius,
    float restitution,
    float mass,
    float staticFriction,
    float dynamicFriction,
    const glm::vec3& initialAngularVelocity)
{
    if (!(mass > 0.0f) || !std::isfinite(mass))
        return false;

    const float inverseMass = 1.0f / mass;
    if (!std::isfinite(inverseMass))
        return false;

    addSphereBodyInternal(
        transform,
        initialVelocity,
        initialAngularVelocity,
        sphereRadius,
        restitution,
        staticFriction,
        dynamicFriction,
        inverseMass);
    return true;
}

bool PhysicsWorld::addBoxBody(
    Transform& transform,
    const glm::vec3& localHalfExtents,
    const glm::vec3& initialVelocity,
    float restitution,
    float mass,
    float staticFriction,
    float dynamicFriction,
    const glm::vec3& initialAngularVelocity)
{
    if (!isFiniteVector(localHalfExtents) ||
        localHalfExtents.x <= 1e-5f ||
        localHalfExtents.y <= 1e-5f ||
        localHalfExtents.z <= 1e-5f ||
        !isFiniteVector(initialVelocity) ||
        !isFiniteVector(initialAngularVelocity) ||
        !(mass > 0.0f) || !std::isfinite(mass))
    {
        return false;
    }

    const float inverseMass = 1.0f / mass;
    if (!std::isfinite(inverseMass))
        return false;

    Body body;
    body.transform = &transform;
    body.collider.shape = BodyShape::Box;
    body.velocity = initialVelocity;
    body.angularVelocity = initialAngularVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    body.startAngularVelocity = initialAngularVelocity;
    body.startOrientation = transform.orientation();
    body.collider.baseBoxHalfExtents = localHalfExtents;
    body.material.restitution = std::clamp(restitution, 0.0f, 1.0f);
    body.material.staticFriction = clampFriction(staticFriction);
    body.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), body.material.staticFriction);
    body.inverseMass = inverseMass;

    bodies_.push_back(body);
    bodies_.back().synchronizeColliderAndInertia();
    return true;
}

void PhysicsWorld::addStaticSphere(
    Transform& transform,
    float sphereRadius,
    float restitution,
    float staticFriction,
    float dynamicFriction)
{
    addSphereBodyInternal(
        transform,
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        sphereRadius,
        restitution,
        staticFriction,
        dynamicFriction,
        0.0f);
}

bool PhysicsWorld::addStaticBox(
    Transform& transform,
    const glm::vec3& localCenter,
    const glm::vec3& localHalfExtents,
    float restitution,
    float staticFriction,
    float dynamicFriction)
{
    if (!isFiniteVector(localCenter) || !isFiniteVector(localHalfExtents) ||
        localHalfExtents.x <= 0.0f || localHalfExtents.y <= 0.0f ||
        localHalfExtents.z <= 0.0f)
    {
        return false;
    }

    for (const StaticBoxCollider& collider : staticBoxes_)
    {
        if (collider.transform == &transform)
            return false;
    }

    StaticBoxCollider collider;
    collider.transform = &transform;
    collider.localCenter = localCenter;
    collider.localHalfExtents = localHalfExtents;
    collider.material.restitution = std::clamp(restitution, 0.0f, 1.0f);
    collider.material.staticFriction = clampFriction(staticFriction);
    collider.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), collider.material.staticFriction);
    staticBoxes_.push_back(collider);
    return true;
}

void PhysicsWorld::addSphereBodyInternal(
    Transform& transform,
    const glm::vec3& initialVelocity,
    const glm::vec3& initialAngularVelocity,
    float sphereRadius,
    float restitution,
    float staticFriction,
    float dynamicFriction,
    float inverseMass)
{
    Body body;
    body.transform = &transform;
    body.collider.shape = BodyShape::Sphere;
    body.velocity = initialVelocity;
    body.angularVelocity = initialAngularVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    body.startAngularVelocity = initialAngularVelocity;
    body.startOrientation = transform.orientation();
    body.collider.baseSphereRadius = std::max(0.0f, sphereRadius);
    body.collider.sphereRadius = body.collider.baseSphereRadius;
    body.material.restitution = std::clamp(restitution, 0.0f, 1.0f);
    body.material.staticFriction = clampFriction(staticFriction);
    body.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), body.material.staticFriction);
    body.inverseMass = inverseMass;

    bodies_.push_back(body);
    bodies_.back().synchronizeColliderAndInertia();
}


PhysicsWorld::Body* PhysicsWorld::findBody(Transform& transform)
{
    for (Body& body : bodies_)
    {
        if (body.transform == &transform)
            return &body;
    }
    return nullptr;
}

const PhysicsWorld::Body* PhysicsWorld::findBody(const Transform& transform) const
{
    for (const Body& body : bodies_)
    {
        if (body.transform == &transform)
            return &body;
    }
    return nullptr;
}

bool PhysicsWorld::applyForce(Transform& transform, const glm::vec3& force)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(force))
        return false;

    body->forceAccumulator += force;
    return true;
}

bool PhysicsWorld::applyForceAtPoint(
    Transform& transform,
    const glm::vec3& force,
    const glm::vec3& worldPoint)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f ||
        !isFiniteVector(force) || !isFiniteVector(worldPoint))
        return false;

    body->forceAccumulator += force;
    body->torqueAccumulator += glm::cross(
        worldPoint - transform.position(), force);
    return true;
}

bool PhysicsWorld::applyTorque(Transform& transform, const glm::vec3& torque)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(torque))
        return false;
    body->synchronizeColliderAndInertia();
    if (body->inverseMass <= 0.0f)
        return false;

    body->torqueAccumulator += torque;
    return true;
}

bool PhysicsWorld::setContinuousForce(
    Transform& transform, const glm::vec3& force)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(force))
        return false;

    body->continuousForce = force;
    return true;
}

bool PhysicsWorld::setContinuousTorque(
    Transform& transform, const glm::vec3& torque)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(torque))
        return false;
    body->synchronizeColliderAndInertia();
    if (body->inverseMass <= 0.0f)
        return false;

    body->continuousTorque = torque;
    return true;
}

bool PhysicsWorld::applyImpulse(Transform& transform, const glm::vec3& impulse)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(impulse))
        return false;

    body->velocity += impulse * body->inverseMass;
    return true;
}

bool PhysicsWorld::applyAngularImpulse(
    Transform& transform, const glm::vec3& angularImpulse)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(angularImpulse))
        return false;
    body->synchronizeColliderAndInertia();

    body->angularVelocity += body->inverseInertiaTensorWorld() * angularImpulse;
    return true;
}

bool PhysicsWorld::applyImpulseAtPoint(
    Transform& transform,
    const glm::vec3& impulse,
    const glm::vec3& worldPoint)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f ||
        !isFiniteVector(impulse) || !isFiniteVector(worldPoint))
        return false;
    body->synchronizeColliderAndInertia();

    const glm::vec3 arm = worldPoint - transform.position();
    body->velocity += impulse * body->inverseMass;
    body->angularVelocity += body->inverseInertiaTensorWorld() * glm::cross(arm, impulse);
    return true;
}

void PhysicsWorld::update(float frameTime)
{
    if (!(frameTime > 0.0f) || !std::isfinite(frameTime))
        return;

    accumulator_ += frameTime;

    int steps = 0;
    while (accumulator_ + kTolerance >= kFixedStep && steps < kMaxStepsPerFrame)
    {
        step(static_cast<float>(kFixedStep));
        accumulator_ -= kFixedStep;
        ++steps;
    }

    // Mantém um limite de trabalho por quadro. O tempo excedente fica limitado
    // a menos de um passo para evitar uma espiral de atualização atrasada.
    if (steps == kMaxStepsPerFrame && accumulator_ >= kFixedStep)
        accumulator_ = std::fmod(accumulator_, kFixedStep);

    // Forças pontuais acumuladas são consumidas quando ao menos um passo fixo
    // foi processado. Forças contínuas permanecem ativas até serem substituídas.
    if (steps > 0)
    {
        for (Body& body : bodies_)
        {
            body.forceAccumulator = glm::vec3(0.0f);
            body.torqueAccumulator = glm::vec3(0.0f);
        }
    }
}

void PhysicsWorld::step(float dt)
{
    float smallestExtent = std::numeric_limits<float>::max();
    float maximumTravel = 0.0f;

    for (Body& body : bodies_)
    {
        body.synchronizeColliderAndInertia();
        if (!body.transform)
            continue;

        const float minimumShapeExtent = body.collider.shape == BodyShape::Sphere
            ? body.collider.sphereRadius
            : std::min(body.collider.boxHalfExtents.x,
                std::min(body.collider.boxHalfExtents.y, body.collider.boxHalfExtents.z));
        if (minimumShapeExtent <= 1e-6f)
            continue;
        if (minimumShapeExtent > 1e-6f)
            smallestExtent = std::min(smallestExtent, minimumShapeExtent);
        if (body.inverseMass <= 0.0f)
            continue;

        const glm::vec3 acceleration = gravity_ +
            (body.forceAccumulator + body.continuousForce) * body.inverseMass;
        const glm::vec3 predictedVelocity = body.velocity + acceleration * dt;
        const float predictedTravel = glm::length(predictedVelocity) * dt +
            0.5f * glm::length(acceleration) * dt * dt;
        maximumTravel = std::max(maximumTravel, predictedTravel);
    }

    // Subpassos adaptativos limitam o deslocamento por subpasso em relação
    // à menor meia-dimensão física (esfera ou caixa), reduzindo tunneling.
    int substeps = 1;
    if (smallestExtent < std::numeric_limits<float>::max())
    {
        const float maximumSubstepTravel = std::max(1e-4f, smallestExtent * 0.5f);
        const float requiredSubsteps = maximumTravel / maximumSubstepTravel;
        if (!std::isfinite(requiredSubsteps) ||
            requiredSubsteps >= static_cast<float>(kMaxAdaptiveSubsteps))
        {
            substeps = kMaxAdaptiveSubsteps;
        }
        else
        {
            substeps = std::max(1, static_cast<int>(std::ceil(requiredSubsteps)));
        }
    }

    const float substep = dt / static_cast<float>(substeps);
    for (int substepIndex = 0; substepIndex < substeps; ++substepIndex)
    {
        for (Body& body : bodies_)
        {
            if (!body.transform || body.inverseMass == 0.0f)
                continue;

            const glm::vec3 acceleration = gravity_ +
                (body.forceAccumulator + body.continuousForce) * body.inverseMass;
            const glm::vec3 angularAcceleration = body.inverseInertiaTensorWorld() *
                (body.torqueAccumulator + body.continuousTorque);

            body.velocity += acceleration * substep;
            body.angularVelocity += angularAcceleration * substep;
            body.transform->translate(body.velocity * substep);

            const float angularSpeed = glm::length(body.angularVelocity);
            if (angularSpeed > 1e-6f)
            {
                const glm::quat delta = glm::angleAxis(
                    angularSpeed * substep, body.angularVelocity / angularSpeed);
                body.transform->setOrientation(glm::normalize(
                    delta * body.transform->orientation()));
            }
        }

        // Detecta uma vez por subpasso e mantém o mesmo conjunto de contatos
        // durante as iterações do solver. Isso permite acumular impulsos por
        // ponto sem recalcular a geometria a cada iteração.
        std::vector<ContactConstraint> contacts;
        contacts.reserve(bodies_.size() * 4);
        collectContactConstraints(contacts);

        // Contatos contra suportes estáticos são resolvidos primeiro. Em pilhas,
        // isso ancora os corpos inferiores antes de propagar as cargas para os
        // corpos dinâmicos apoiados neles nas iterações seguintes.
        std::stable_partition(
            contacts.begin(), contacts.end(),
            [](const ContactConstraint& contact)
            {
                return !contact.bodyA || !contact.bodyB ||
                    contact.bodyA->inverseMass <= 0.0f ||
                    contact.bodyB->inverseMass <= 0.0f;
            });
        for (ContactConstraint& contact : contacts)
        {
            solver::prepareContactManifold(
                contact.bodyA,
                contact.bodyB,
                contact.manifold,
                contact.material);
        }

        for (int iteration = 0; iteration < kCollisionIterations; ++iteration)
        {
            for (ContactConstraint& contact : contacts)
            {
                solver::solveContactVelocity(
                    contact.bodyA,
                    contact.bodyB,
                    contact.manifold,
                    contact.material);
            }
        }

        // Projeção posicional separada dos impulsos. Recoletamos os contatos
        // após cada passagem, porque corrigir um apoio muda imediatamente os
        // contatos dos corpos empilhados acima dele. Suportes estáticos são
        // corrigidos primeiro; pares dinâmicos vêm em seguida.
        for (int iteration = 0; iteration < kPositionIterations; ++iteration)
        {
            std::vector<ContactConstraint> positionContacts;
            positionContacts.reserve(bodies_.size() * 4);
            collectContactConstraints(positionContacts);
            if (positionContacts.empty())
                break;

            std::stable_partition(
                positionContacts.begin(), positionContacts.end(),
                [](const ContactConstraint& contact)
                {
                    return !contact.bodyA || !contact.bodyB ||
                        contact.bodyA->inverseMass <= 0.0f ||
                        contact.bodyB->inverseMass <= 0.0f;
                });

            float deepestPenetration = 0.0f;
            for (const ContactConstraint& contact : positionContacts)
            {
                for (std::size_t point = 0;
                     point < contact.manifold.pointCount; ++point)
                {
                    deepestPenetration = std::max(
                        deepestPenetration,
                        contact.manifold.points[point].penetration);
                }

                solver::correctContactPosition(
                    contact.bodyA,
                    contact.bodyB,
                    contact.manifold,
                    1.0f);
            }

            if (deepestPenetration <= 0.0002f)
                break;
        }
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
        body.transform->setOrientation(body.startOrientation);
        body.velocity = body.startVelocity;
        body.angularVelocity = body.startAngularVelocity;
        body.forceAccumulator = glm::vec3(0.0f);
        body.torqueAccumulator = glm::vec3(0.0f);
        body.continuousForce = glm::vec3(0.0f);
        body.continuousTorque = glm::vec3(0.0f);
        body.synchronizeColliderAndInertia();
    }

    accumulator_ = 0.0;
}

void PhysicsWorld::captureInitialPose()
{
    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        body.synchronizeColliderAndInertia();
        body.startPosition = body.transform->position();
        body.startOrientation = body.transform->orientation();
    }
}

void PhysicsWorld::clear()
{
    bodies_.clear();
    staticBoxes_.clear();
    clearGroundPlane();
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

bool PhysicsWorld::hasDynamicBody(const Transform& transform) const
{
    const Body* body = findBody(transform);
    return body && body->inverseMass > 0.0f;
}

glm::vec3 PhysicsWorld::linearVelocity(const Transform& transform) const
{
    const Body* body = findBody(transform);
    return body ? body->velocity : glm::vec3(0.0f);
}

glm::vec3 PhysicsWorld::angularVelocity(const Transform& transform) const
{
    const Body* body = findBody(transform);
    return body ? body->angularVelocity : glm::vec3(0.0f);
}

} // namespace rotor::physics
