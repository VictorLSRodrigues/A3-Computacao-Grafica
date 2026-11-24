#include "physics_world.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
float clampFriction(float coefficient)
{
    return std::clamp(coefficient, 0.0f, 1.0f);
}

glm::vec3 velocityAtPoint(
    const glm::vec3& linearVelocity,
    const glm::vec3& angularVelocity,
    const glm::vec3& arm)
{
    return linearVelocity + glm::cross(angularVelocity, arm);
}

float effectiveMassDenominator(
    float inverseMassA,
    float inverseInertiaA,
    const glm::vec3& armA,
    float inverseMassB,
    float inverseInertiaB,
    const glm::vec3& armB,
    const glm::vec3& direction)
{
    const glm::vec3 angularA = glm::cross(armA, direction);
    const glm::vec3 angularB = glm::cross(armB, direction);

    return inverseMassA + inverseMassB +
        inverseInertiaA * glm::dot(angularA, angularA) +
        inverseInertiaB * glm::dot(angularB, angularB);
}

float combineFriction(float a, float b)
{
    return std::sqrt(std::max(0.0f, a * b));
}

bool isFiniteVector(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}

constexpr float kBowlLipStart = 0.78f;

void evaluateBowlProfile(
    float radius,
    float depth,
    float lipHeight,
    float radialDistance,
    float& height,
    float& slope,
    float& curvature)
{
    const float t = std::clamp(radialDistance / radius, 0.0f, 1.0f);
    const float inverseRadius = 1.0f / radius;
    const float lipWidth = 1.0f - kBowlLipStart;

    height = depth * t * t;
    slope = 2.0f * depth * radialDistance * inverseRadius * inverseRadius;
    curvature = 2.0f * depth * inverseRadius * inverseRadius;

    if (t > kBowlLipStart && lipHeight > 0.0f)
    {
        const float u = (t - kBowlLipStart) / lipWidth;
        const float u2 = u * u;
        const float u3 = u2 * u;
        height += lipHeight * u2 * u2;
        slope += 4.0f * lipHeight * u3 / (radius * lipWidth);
        curvature += 12.0f * lipHeight * u2 /
            (radius * radius * lipWidth * lipWidth);
    }
}

struct BowlContact
{
    glm::vec3 surfacePoint{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float signedDistance = 0.0f;
};

BowlContact findClosestBowlSurface(
    const glm::vec3& position,
    float radius,
    float depth,
    float lipHeight)
{
    const float radialDistance = std::sqrt(
        position.x * position.x + position.z * position.z);
    const float directionX = radialDistance > 1e-6f
        ? position.x / radialDistance : 1.0f;
    const float directionZ = radialDistance > 1e-6f
        ? position.z / radialDistance : 0.0f;

    // Para uma superfície de revolução, o ponto mais próximo compartilha o
    // azimute do centro da esfera. Newton resolve a coordenada radial s para
    // que o segmento até a superfície fique alinhado à normal local.
    float surfaceRadius = std::clamp(radialDistance, 0.0f, radius);
    for (int iteration = 0; iteration < 10; ++iteration)
    {
        float height = 0.0f;
        float slope = 0.0f;
        float curvature = 0.0f;
        evaluateBowlProfile(
            radius, depth, lipHeight, surfaceRadius,
            height, slope, curvature);

        const float function = (surfaceRadius - radialDistance) +
            (height - position.y) * slope;
        const float derivative = 1.0f + slope * slope +
            (height - position.y) * curvature;
        if (std::fabs(derivative) <= 1e-6f)
            break;

        const float nextRadius = std::clamp(
            surfaceRadius - function / derivative, 0.0f, radius);
        if (std::fabs(nextRadius - surfaceRadius) <= 1e-5f)
        {
            surfaceRadius = nextRadius;
            break;
        }
        surfaceRadius = nextRadius;
    }

    float surfaceHeight = 0.0f;
    float surfaceSlope = 0.0f;
    float ignoredCurvature = 0.0f;
    evaluateBowlProfile(
        radius, depth, lipHeight, surfaceRadius,
        surfaceHeight, surfaceSlope, ignoredCurvature);

    BowlContact contact;
    contact.surfacePoint = glm::vec3(
        directionX * surfaceRadius,
        surfaceHeight,
        directionZ * surfaceRadius);
    contact.normal = glm::normalize(glm::vec3(
        -surfaceSlope * directionX,
        1.0f,
        -surfaceSlope * directionZ));
    contact.signedDistance = glm::dot(
        position - contact.surfacePoint, contact.normal);
    return contact;
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

bool PhysicsWorld::addBody(
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

    addBodyInternal(
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

void PhysicsWorld::addStaticBody(
    Transform& transform,
    float sphereRadius,
    float restitution,
    float staticFriction,
    float dynamicFriction)
{
    addBodyInternal(
        transform,
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        sphereRadius,
        restitution,
        staticFriction,
        dynamicFriction,
        0.0f);
}

void PhysicsWorld::addBodyInternal(
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
    body.velocity = initialVelocity;
    body.angularVelocity = initialAngularVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    body.startAngularVelocity = initialAngularVelocity;
    body.startOrientation = transform.orientation();
    body.baseSphereRadius = std::max(0.0f, sphereRadius);
    body.sphereRadius = body.baseSphereRadius;
    body.restitution = std::clamp(restitution, 0.0f, 1.0f);
    body.staticFriction = clampFriction(staticFriction);
    body.dynamicFriction = std::min(
        clampFriction(dynamicFriction), body.staticFriction);
    body.inverseMass = inverseMass;

    bodies_.push_back(body);
    synchronizeBodyScale(bodies_.back());
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
    synchronizeBodyScale(*body);
    if (body->inverseInertia <= 0.0f)
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
    synchronizeBodyScale(*body);
    if (body->inverseInertia <= 0.0f)
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
    synchronizeBodyScale(*body);
    if (body->inverseInertia <= 0.0f)
        return false;

    body->angularVelocity += angularImpulse * body->inverseInertia;
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
    synchronizeBodyScale(*body);

    const glm::vec3 arm = worldPoint - transform.position();
    body->velocity += impulse * body->inverseMass;
    body->angularVelocity += body->inverseInertia * glm::cross(arm, impulse);
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

void PhysicsWorld::synchronizeBodyScale(Body& body)
{
    if (!body.transform || body.baseSphereRadius <= 0.0f)
    {
        body.sphereRadius = 0.0f;
        body.inverseInertia = 0.0f;
        return;
    }

    // O colisor esférico acompanha a maior escala absoluta. Para escala
    // não uniforme, isso produz uma esfera envolvente conservadora.
    const glm::vec3 scale = glm::abs(body.transform->scale());
    const float scaleFactor = std::max(scale.x, std::max(scale.y, scale.z));
    const float scaledRadius = body.baseSphereRadius * scaleFactor;
    if (!std::isfinite(scaleFactor) || !std::isfinite(scaledRadius))
    {
        body.sphereRadius = 0.0f;
        body.inverseInertia = 0.0f;
        return;
    }
    body.sphereRadius = scaledRadius;

    if (body.inverseMass > 0.0f && body.sphereRadius > 1e-6f)
    {
        const float radiusSquared = body.sphereRadius * body.sphereRadius;
        body.inverseInertia = 2.5f * body.inverseMass / radiusSquared;
    }
    else
    {
        body.inverseInertia = 0.0f;
    }
}

void PhysicsWorld::step(float dt)
{
    float smallestRadius = std::numeric_limits<float>::max();
    float maximumTravel = 0.0f;

    for (Body& body : bodies_)
    {
        synchronizeBodyScale(body);
        if (!body.transform || body.sphereRadius <= 0.0f)
            continue;

        smallestRadius = std::min(smallestRadius, body.sphereRadius);
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
    // ao menor raio físico, reduzindo tunneling em colisões de alta velocidade.
    int substeps = 1;
    if (smallestRadius < std::numeric_limits<float>::max())
    {
        const float maximumSubstepTravel = std::max(1e-4f, smallestRadius * 0.5f);
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
            const glm::vec3 angularAcceleration =
                (body.torqueAccumulator + body.continuousTorque) * body.inverseInertia;

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

        // Resolve pares primeiro e o chão por último em cada iteração, garantindo
        // que o estado final do subpasso não deixe a esfera enterrada na superfície.
        for (int iteration = 0; iteration < kCollisionIterations; ++iteration)
        {
            resolveSphereCollisions();
            resolveGroundCollisions();
        }
    }

    ++stepCount_;
}

void PhysicsWorld::resolveGroundCollisions()
{
    if (!hasGroundPlane_ && !hasGroundBowl_)
        return;

    for (Body& body : bodies_)
    {
        if (!body.transform || body.inverseMass == 0.0f || body.sphereRadius <= 0.0f)
            continue;

        glm::vec3 position = body.transform->position();
        glm::vec3 normal(0.0f, 1.0f, 0.0f);
        float signedDistance = 0.0f;

        if (hasGroundPlane_)
        {
            signedDistance = position.y - groundHeight_;
        }
        else
        {
            const BowlContact contact = findClosestBowlSurface(
                position,
                groundBowlRadius_,
                groundBowlDepth_,
                groundBowlLipHeight_);
            normal = contact.normal;
            signedDistance = contact.signedDistance;
        }

        if (signedDistance >= body.sphereRadius)
            continue;

        // Corrige a penetração ao longo da normal local da superfície. Na
        // bacia, a normal muda com a posição e aponta para dentro do recipiente.
        position += normal * (body.sphereRadius - signedDistance);
        body.transform->move(position);

        const glm::vec3 arm = -normal * body.sphereRadius;
        glm::vec3 contactVelocity = velocityAtPoint(
            body.velocity, body.angularVelocity, arm);
        const float normalVelocity = glm::dot(contactVelocity, normal);
        if (normalVelocity >= 0.0f)
            continue;

        const float restitution = -normalVelocity > kRestitutionVelocityThreshold
            ? body.restitution
            : 0.0f;
        const float normalDenominator = effectiveMassDenominator(
            body.inverseMass, body.inverseInertia, arm,
            0.0f, 0.0f, glm::vec3(0.0f), normal);
        if (normalDenominator <= 1e-8f)
            continue;

        const float normalImpulseMagnitude =
            -(1.0f + restitution) * normalVelocity / normalDenominator;
        const glm::vec3 normalImpulse = normal * normalImpulseMagnitude;

        body.velocity += normalImpulse * body.inverseMass;
        body.angularVelocity += body.inverseInertia * glm::cross(arm, normalImpulse);

        // O atrito usa o plano tangente local e produz torque em qualquer
        // orientação da superfície, inclusive nas laterais inclinadas da bacia.
        contactVelocity = velocityAtPoint(
            body.velocity, body.angularVelocity, arm);
        const glm::vec3 tangentVelocity =
            contactVelocity - glm::dot(contactVelocity, normal) * normal;
        const float tangentSpeed = glm::length(tangentVelocity);
        if (tangentSpeed <= 1e-6f)
            continue;

        const glm::vec3 tangent = tangentVelocity / tangentSpeed;
        const float tangentDenominator = effectiveMassDenominator(
            body.inverseMass, body.inverseInertia, arm,
            0.0f, 0.0f, glm::vec3(0.0f), tangent);
        if (tangentDenominator <= 1e-8f)
            continue;

        const float requiredImpulse = -glm::dot(contactVelocity, tangent) /
            tangentDenominator;
        const float staticFriction = combineFriction(
            body.staticFriction, groundStaticFriction_);
        const float dynamicFriction = combineFriction(
            body.dynamicFriction, groundDynamicFriction_);
        const float staticLimit = staticFriction * normalImpulseMagnitude;

        const float tangentImpulseMagnitude =
            std::fabs(requiredImpulse) <= staticLimit
                ? requiredImpulse
                : -dynamicFriction * normalImpulseMagnitude;
        const glm::vec3 tangentImpulse = tangent * tangentImpulseMagnitude;

        body.velocity += tangentImpulse * body.inverseMass;
        body.angularVelocity += body.inverseInertia * glm::cross(arm, tangentImpulse);
    }
}

void PhysicsWorld::resolveSphereCollisions()
{
    for (std::size_t i = 0; i < bodies_.size(); ++i)
    {
        Body& bodyA = bodies_[i];
        if (!bodyA.transform || bodyA.sphereRadius <= 0.0f)
            continue;

        for (std::size_t j = i + 1; j < bodies_.size(); ++j)
        {
            Body& bodyB = bodies_[j];
            if (!bodyB.transform || bodyB.sphereRadius <= 0.0f)
                continue;

            const float inverseMassSum = bodyA.inverseMass + bodyB.inverseMass;
            if (inverseMassSum <= 0.0f)
                continue;

            const glm::vec3 delta =
                bodyB.transform->position() - bodyA.transform->position();
            const float distanceSquared = glm::dot(delta, delta);
            const float combinedRadius = bodyA.sphereRadius + bodyB.sphereRadius;
            if (distanceSquared >= combinedRadius * combinedRadius)
                continue;

            const float distance = std::sqrt(std::max(0.0f, distanceSquared));
            const glm::vec3 normal = distance > 1e-6f
                ? delta / distance
                : glm::vec3(1.0f, 0.0f, 0.0f);
            const float penetration = combinedRadius - distance;
            const float correctionMagnitude =
                std::max(penetration - kPenetrationSlop, 0.0f) *
                kPositionCorrection / inverseMassSum;
            const glm::vec3 correction = normal * correctionMagnitude;

            if (bodyA.inverseMass > 0.0f)
                bodyA.transform->translate(-correction * bodyA.inverseMass);
            if (bodyB.inverseMass > 0.0f)
                bodyB.transform->translate(correction * bodyB.inverseMass);

            const glm::vec3 armA = normal * bodyA.sphereRadius;
            const glm::vec3 armB = -normal * bodyB.sphereRadius;
            glm::vec3 relativeVelocity =
                velocityAtPoint(bodyB.velocity, bodyB.angularVelocity, armB) -
                velocityAtPoint(bodyA.velocity, bodyA.angularVelocity, armA);
            const float normalVelocity = glm::dot(relativeVelocity, normal);
            if (normalVelocity >= 0.0f)
                continue;

            const float restitution = -normalVelocity > kRestitutionVelocityThreshold
                ? std::min(bodyA.restitution, bodyB.restitution)
                : 0.0f;
            const float normalDenominator = effectiveMassDenominator(
                bodyA.inverseMass, bodyA.inverseInertia, armA,
                bodyB.inverseMass, bodyB.inverseInertia, armB,
                normal);
            if (normalDenominator <= 1e-8f)
                continue;

            const float normalImpulseMagnitude =
                -(1.0f + restitution) * normalVelocity / normalDenominator;
            const glm::vec3 normalImpulse = normal * normalImpulseMagnitude;

            bodyA.velocity -= normalImpulse * bodyA.inverseMass;
            bodyA.angularVelocity -= bodyA.inverseInertia * glm::cross(armA, normalImpulse);
            bodyB.velocity += normalImpulse * bodyB.inverseMass;
            bodyB.angularVelocity += bodyB.inverseInertia * glm::cross(armB, normalImpulse);

            // Recalcula a velocidade relativa no contato após o impulso normal.
            relativeVelocity =
                velocityAtPoint(bodyB.velocity, bodyB.angularVelocity, armB) -
                velocityAtPoint(bodyA.velocity, bodyA.angularVelocity, armA);
            const glm::vec3 tangentVelocity =
                relativeVelocity - glm::dot(relativeVelocity, normal) * normal;
            const float tangentSpeed = glm::length(tangentVelocity);
            if (tangentSpeed <= 1e-6f)
                continue;

            const glm::vec3 tangent = tangentVelocity / tangentSpeed;
            const float tangentDenominator = effectiveMassDenominator(
                bodyA.inverseMass, bodyA.inverseInertia, armA,
                bodyB.inverseMass, bodyB.inverseInertia, armB,
                tangent);
            if (tangentDenominator <= 1e-8f)
                continue;

            const float requiredImpulse = -glm::dot(relativeVelocity, tangent) /
                tangentDenominator;
            const float staticFriction = combineFriction(
                bodyA.staticFriction, bodyB.staticFriction);
            const float dynamicFriction = combineFriction(
                bodyA.dynamicFriction, bodyB.dynamicFriction);
            const float staticLimit = staticFriction * normalImpulseMagnitude;
            const float tangentImpulseMagnitude =
                std::fabs(requiredImpulse) <= staticLimit
                    ? requiredImpulse
                    : -dynamicFriction * normalImpulseMagnitude;
            const glm::vec3 tangentImpulse = tangent * tangentImpulseMagnitude;

            bodyA.velocity -= tangentImpulse * bodyA.inverseMass;
            bodyA.angularVelocity -= bodyA.inverseInertia * glm::cross(armA, tangentImpulse);
            bodyB.velocity += tangentImpulse * bodyB.inverseMass;
            bodyB.angularVelocity += bodyB.inverseInertia * glm::cross(armB, tangentImpulse);
        }
    }
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
        synchronizeBodyScale(body);
    }

    accumulator_ = 0.0;
}

void PhysicsWorld::recordStartPositions()
{
    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        synchronizeBodyScale(body);
        body.startPosition = body.transform->position();
        body.startOrientation = body.transform->orientation();
    }
}

void PhysicsWorld::clear()
{
    bodies_.clear();
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
