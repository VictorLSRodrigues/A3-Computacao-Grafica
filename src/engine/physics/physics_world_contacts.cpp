#include "physics_world.h"

#include "engine/physics/collision/collision_geometry.h"
#include "engine/physics/solver/contact_solver.h"

#include <algorithm>
#include <cmath>

namespace rotor::physics
{
using collision::OrientedBox;

namespace
{
void setSinglePointManifold(
    ContactManifold& manifold,
    const ContactPoint& point)
{
    manifold.clear();
    manifold.points[0] = point;
    manifold.pointCount = 1;
}

PhysicsMaterial groundMaterialFor(
    const RigidBody& body,
    float staticFriction,
    float dynamicFriction)
{
    PhysicsMaterial material;
    material.restitution = body.material.restitution;
    material.staticFriction = staticFriction;
    material.dynamicFriction = dynamicFriction;
    return rotor::physics::solver::combineMaterials(body.material, material);
}
} // namespace

void PhysicsWorld::collectContactConstraints(
    std::vector<ContactConstraint>& contacts)
{
    contacts.clear();

    // 1. Contatos entre corpos dinâmicos/estáticos registrados como bodies.
    for (std::size_t i = 0; i < bodies_.size(); ++i)
    {
        Body& bodyA = bodies_[i];
        if (!bodyA.transform)
            continue;

        for (std::size_t j = i + 1; j < bodies_.size(); ++j)
        {
            Body& bodyB = bodies_[j];
            if (!bodyB.transform ||
                bodyA.inverseMass + bodyB.inverseMass <= 0.0f)
                continue;

            ContactManifold manifold;
            bool collided = false;

            if (bodyA.collider.shape == BodyShape::Sphere &&
                bodyB.collider.shape == BodyShape::Sphere)
            {
                ContactPoint point;
                collided = collision::sphereSphereContact(
                    bodyA.transform->position(), bodyA.collider.sphereRadius,
                    bodyB.transform->position(), bodyB.collider.sphereRadius,
                    point);
                if (collided)
                    setSinglePointManifold(manifold, point);
            }
            else if (bodyA.collider.shape == BodyShape::Box &&
                     bodyB.collider.shape == BodyShape::Box)
            {
                const OrientedBox boxA = collision::makeOrientedBox(
                    bodyA.transform->position(), bodyA.transform->orientation(),
                    bodyA.collider.boxHalfExtents);
                const OrientedBox boxB = collision::makeOrientedBox(
                    bodyB.transform->position(), bodyB.transform->orientation(),
                    bodyB.collider.boxHalfExtents);
                collided = collision::intersectOrientedBoxes(boxA, boxB, manifold);
            }
            else
            {
                const Body* sphereBody = bodyA.collider.shape == BodyShape::Sphere
                    ? &bodyA : &bodyB;
                const Body* boxBody = bodyA.collider.shape == BodyShape::Box
                    ? &bodyA : &bodyB;
                const OrientedBox box = collision::makeOrientedBox(
                    boxBody->transform->position(), boxBody->transform->orientation(),
                    boxBody->collider.boxHalfExtents);
                ContactPoint point;
                collided = collision::sphereOrientedBoxContact(
                    sphereBody->transform->position(),
                    sphereBody->collider.sphereRadius, box, point);
                if (collided)
                {
                    // A consulta esfera-caixa produz normal caixa -> esfera.
                    // O solver de pares espera normal de A para B.
                    if (sphereBody == &bodyA)
                    {
                        point.normal = -point.normal;
                        std::swap(point.pointA, point.pointB);
                    }
                    setSinglePointManifold(manifold, point);
                }
            }

            if (!collided || manifold.empty())
                continue;

            contacts.push_back(ContactConstraint{
                &bodyA,
                &bodyB,
                manifold,
                solver::combineMaterials(bodyA.material, bodyB.material)});
        }
    }

    // 2. Contatos contra caixas estáticas, como o tampo da mesa.
    for (Body& body : bodies_)
    {
        if (!body.transform || body.inverseMass <= 0.0f)
            continue;

        for (const StaticBoxCollider& collider : staticBoxes_)
        {
            if (!collider.transform)
                continue;

            const glm::vec3 scale = collider.transform->scale();
            const glm::vec3 absoluteScale = glm::abs(scale);
            if (absoluteScale.x <= 1e-6f || absoluteScale.y <= 1e-6f ||
                absoluteScale.z <= 1e-6f)
                continue;

            const glm::quat orientation = collider.transform->orientation();
            const glm::vec3 boxCenter = collider.transform->position() +
                orientation * (scale * collider.localCenter);
            const OrientedBox staticBox = collision::makeOrientedBox(
                boxCenter, orientation, absoluteScale * collider.localHalfExtents);

            ContactManifold manifold;
            bool collided = false;
            if (body.collider.shape == BodyShape::Sphere)
            {
                ContactPoint point;
                collided = collision::sphereOrientedBoxContact(
                    body.transform->position(), body.collider.sphereRadius,
                    staticBox, point);
                if (collided)
                    setSinglePointManifold(manifold, point);
            }
            else
            {
                const OrientedBox dynamicBox = collision::makeOrientedBox(
                    body.transform->position(), body.transform->orientation(),
                    body.collider.boxHalfExtents);
                collided = collision::intersectOrientedBoxes(
                    staticBox, dynamicBox, manifold);
            }

            if (!collided || manifold.empty())
                continue;

            contacts.push_back(ContactConstraint{
                nullptr,
                &body,
                manifold,
                solver::combineMaterials(body.material, collider.material)});
        }
    }

    // 3. Contatos com o chão plano ou a bacia.
    if (!hasGroundPlane_ && !hasGroundBowl_)
        return;

    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    for (Body& body : bodies_)
    {
        if (!body.transform || body.inverseMass <= 0.0f)
            continue;

        ContactManifold manifold;
        if (hasGroundPlane_ && body.collider.shape == BodyShape::Box)
        {
            const OrientedBox box = collision::makeOrientedBox(
                body.transform->position(), body.transform->orientation(),
                body.collider.boxHalfExtents);
            collision::boxPlaneContacts(
                box, worldUp, groundHeight_, manifold);
        }
        else if (hasGroundPlane_)
        {
            const glm::vec3 position = body.transform->position();
            const float signedDistance = position.y - groundHeight_;
            const float radius = collision::supportRadius(body, worldUp);
            const float penetration = radius - signedDistance;
            if (penetration > 0.0f)
            {
                ContactPoint point;
                point.normal = worldUp;
                point.penetration = penetration;
                point.pointB = collision::supportPoint(body, -worldUp);
                const float bodyPointDistance = point.pointB.y - groundHeight_;
                point.pointA = point.pointB - worldUp * bodyPointDistance;
                point.point = (point.pointA + point.pointB) * 0.5f;
                point.hasSeparatePoints = true;
                setSinglePointManifold(manifold, point);
            }
        }
        else
        {
            const collision::BowlSurfaceSample surface =
                collision::findClosestBowlSurface(
                    body.transform->position(),
                    groundBowlRadius_,
                    groundBowlDepth_,
                    groundBowlLipHeight_);
            const float support = collision::supportRadius(body, surface.normal);
            const float penetration = support - surface.signedDistance;
            if (penetration > 0.0f)
            {
                ContactPoint point;
                point.normal = surface.normal;
                point.penetration = penetration;
                point.pointB = collision::supportPoint(body, -surface.normal);
                const float bodyPointDistance = glm::dot(
                    point.pointB - surface.surfacePoint, surface.normal);
                point.pointA = point.pointB - surface.normal * bodyPointDistance;
                point.point = (point.pointA + point.pointB) * 0.5f;
                point.hasSeparatePoints = true;
                setSinglePointManifold(manifold, point);
            }
        }

        if (manifold.empty())
            continue;

        contacts.push_back(ContactConstraint{
            nullptr,
            &body,
            manifold,
            groundMaterialFor(
                body, groundStaticFriction_, groundDynamicFriction_)});
    }
}

void PhysicsWorld::stabilizeInitialOverlaps(int iterations)
{
    const int boundedIterations = std::clamp(iterations, 0, 128);
    std::vector<ContactConstraint> contacts;
    contacts.reserve(bodies_.size() * 4);

    // Apenas corrige posições. Não aplica impulsos nem cria velocidades iniciais.
    // O estado corrigido passa a ser a pose de reset capturada pela aplicação.
    for (int iteration = 0; iteration < boundedIterations; ++iteration)
    {
        collectContactConstraints(contacts);
        if (contacts.empty())
            break;

        // Corrige primeiro corpos apoiados em superfícies estáticas, antes de
        // propagar a correção aos pares dinâmicos empilhados acima deles.
        std::stable_partition(
            contacts.begin(), contacts.end(),
            [](const ContactConstraint& constraint)
            {
                return !constraint.bodyA || !constraint.bodyB ||
                    constraint.bodyA->inverseMass <= 0.0f ||
                    constraint.bodyB->inverseMass <= 0.0f;
            });

        float deepestPenetration = 0.0f;
        for (const ContactConstraint& constraint : contacts)
        {
            for (std::size_t i = 0; i < constraint.manifold.pointCount; ++i)
            {
                deepestPenetration = std::max(
                    deepestPenetration,
                    constraint.manifold.points[i].penetration);
            }

            solver::correctContactPosition(
                constraint.bodyA,
                constraint.bodyB,
                constraint.manifold,
                1.0f);
        }

        if (deepestPenetration <= 0.0002f)
            break;
    }
}

} // namespace rotor::physics
