#include <algorithm>
#include "engine/physics/physics_world.h"
#include "engine/graphics/geometry/shapes.h"
#include "engine/physics/collision/collision_geometry.h"
#include "engine/physics/mesh_collider.h"

using rotor::physics::PhysicsWorld;

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <iterator>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    bool passed = true;

    PhysicsWorld duplicateRegistrationWorld;
    Transform duplicateTransform;
    passed &= expect(
        duplicateRegistrationWorld.addSphereBody(duplicateTransform) &&
            !duplicateRegistrationWorld.addBoxBody(
                duplicateTransform, glm::vec3(0.5f)),
        "a Transform cannot be registered as multiple dynamic rigid bodies");
    Transform invalidSphereTransform;
    passed &= expect(
        !duplicateRegistrationWorld.addSphereBody(
            invalidSphereTransform,
            glm::vec3(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f),
            0.5f),
        "non-finite initial velocity is rejected when registering a sphere");
    PhysicsWorld mixedDuplicateWorld;
    Transform mixedDuplicateTransform;
    passed &= expect(
        mixedDuplicateWorld.addStaticBox(
            mixedDuplicateTransform, glm::vec3(0.0f), glm::vec3(1.0f)) &&
            !mixedDuplicateWorld.addSphereBody(mixedDuplicateTransform),
        "a Transform cannot be shared by static and dynamic colliders");

    PhysicsWorld bouncingWorld;
    bouncingWorld.setGravity(glm::vec3(0.0f));
    bouncingWorld.setGroundPlane(0.0f);

    Transform bouncingSphere;
    bouncingSphere.move(glm::vec3(0.0f, 0.51f, 0.0f));
    bouncingWorld.addSphereBody(
        bouncingSphere, glm::vec3(0.0f, -2.0f, 0.0f), 0.5f, 0.8f);
    bouncingWorld.update(1.0f / 120.0f);

    passed &= expect(
        std::fabs(bouncingSphere.position().y - 0.5f) < 2e-4f,
        "sphere is moved out of the ground after contact");

    bouncingWorld.update(1.0f / 120.0f);
    passed &= expect(
        bouncingSphere.position().y > 0.5f,
        "positive restitution makes the sphere bounce");

    bouncingWorld.reset();
    passed &= expect(
        std::fabs(bouncingSphere.position().y - 0.51f) < 1e-5f,
        "reset restores the sphere's starting position");

    PhysicsWorld restingWorld;
    restingWorld.setGroundPlane(0.0f);
    Transform restingSphere;
    restingSphere.move(glm::vec3(0.0f, 1.0f, 0.0f));
    restingWorld.addSphereBody(restingSphere, glm::vec3(0.0f), 0.5f, 0.0f);

    for (int i = 0; i < 240; ++i)
        restingWorld.update(1.0f / 120.0f);

    passed &= expect(
        std::fabs(restingSphere.position().y - 0.5f) < 2e-4f,
        "zero restitution prevents penetration and leaves the sphere resting");

    PhysicsWorld pointWorld;
    pointWorld.setGravity(glm::vec3(0.0f));
    pointWorld.setGroundPlane(0.0f);
    Transform pointBody;
    pointBody.move(glm::vec3(0.0f, 0.01f, 0.0f));
    pointWorld.addSphereBody(pointBody, glm::vec3(0.0f, -2.0f, 0.0f));
    pointWorld.update(1.0f / 120.0f);
    passed &= expect(
        pointBody.position().y < 0.01f,
        "bodies without a sphere radius keep their previous non-colliding behavior");

    PhysicsWorld massWorld;
    massWorld.setGravity(glm::vec3(0.0f));
    Transform lightSphere;
    Transform heavySphere;
    lightSphere.move(glm::vec3(-0.5f, 0.0f, 0.0f));
    heavySphere.move(glm::vec3(0.5f, 0.0f, 0.0f));
    passed &= expect(
        massWorld.addSphereBody(
            lightSphere, glm::vec3(1.0f, 0.0f, 0.0f), 0.5f, 1.0f, 1.0f),
        "positive-mass dynamic body is accepted");
    passed &= expect(
        massWorld.addSphereBody(
            heavySphere, glm::vec3(0.0f), 0.5f, 1.0f, 3.0f),
        "body with a different positive mass is accepted");
    massWorld.update(1.0f / 120.0f);
    massWorld.update(1.0f / 120.0f);
    passed &= expect(
        lightSphere.position().x < -0.5f && heavySphere.position().x > 0.5f,
        "elastic sphere collision transfers momentum according to body mass");

    PhysicsWorld staticWorld;
    Transform fixedSphere;
    Transform movingSphere;
    fixedSphere.move(glm::vec3(0.0f));
    movingSphere.move(glm::vec3(-0.9f, 0.0f, 0.0f));
    staticWorld.addStaticSphere(fixedSphere, 0.5f, 1.0f);
    passed &= expect(
        staticWorld.addSphereBody(
            movingSphere, glm::vec3(1.0f, 0.0f, 0.0f), 0.5f, 1.0f),
        "dynamic sphere is accepted beside a static sphere");
    staticWorld.update(1.0f / 120.0f);
    passed &= expect(
        glm::length(fixedSphere.position()) < 1e-5f,
        "static body is not moved by gravity or collision response");
    staticWorld.update(1.0f / 120.0f);
    passed &= expect(
        movingSphere.position().x < -1.0f,
        "dynamic sphere rebounds from a static sphere");

    PhysicsWorld sphereFrictionWorld;
    sphereFrictionWorld.setGravity(glm::vec3(0.0f));
    Transform sphereFrictionObstacle;
    Transform sphereFrictionBody;
    // Sem sobreposição com o chão: testa somente o atrito no contato entre esferas.
    sphereFrictionObstacle.move(glm::vec3(0.0f, 0.5f, 0.0f));
    sphereFrictionBody.move(glm::vec3(-0.9f, 0.5f, 0.0f));
    sphereFrictionWorld.addStaticSphere(
        sphereFrictionObstacle, 0.5f, 0.0f, 1.0f, 1.0f);
    sphereFrictionWorld.addSphereBody(
        sphereFrictionBody,
        glm::vec3(1.0f, 0.5f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        1.0f,
        1.0f);
    sphereFrictionWorld.update(1.0f / 120.0f);
    sphereFrictionWorld.update(1.0f / 120.0f);
    const glm::vec3 pairNormal = glm::normalize(
        sphereFrictionBody.position() - sphereFrictionObstacle.position());
    const glm::vec3 pairContactArm = -pairNormal * 0.5f;
    const glm::vec3 pairContactVelocity =
        sphereFrictionWorld.linearVelocity(sphereFrictionBody) +
        glm::cross(
            sphereFrictionWorld.angularVelocity(sphereFrictionBody),
            pairContactArm);
    const glm::vec3 pairTangentialVelocity = pairContactVelocity -
        glm::dot(pairContactVelocity, pairNormal) * pairNormal;
    passed &= expect(
        glm::length(pairTangentialVelocity) < 1e-4f,
        "static friction removes tangential relative velocity at sphere contact");
    passed &= expect(
        glm::length(sphereFrictionWorld.angularVelocity(sphereFrictionBody)) > 0.1f,
        "tangential sphere contact generates angular velocity");

    PhysicsWorld slidingWorld;
    slidingWorld.setGroundPlane(0.0f, 0.8f, 0.6f);
    Transform slidingSphere;
    slidingSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    slidingWorld.addSphereBody(
        slidingSphere,
        glm::vec3(3.0f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.8f,
        0.6f);

    PhysicsWorld frictionlessWorld;
    frictionlessWorld.setGroundPlane(0.0f, 0.0f, 0.0f);
    Transform frictionlessSphere;
    frictionlessSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    frictionlessWorld.addSphereBody(
        frictionlessSphere,
        glm::vec3(3.0f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f);

    for (int i = 0; i < 240; ++i)
    {
        slidingWorld.update(1.0f / 120.0f);
        frictionlessWorld.update(1.0f / 120.0f);
    }

    const glm::vec3 rollingLinearVelocity =
        slidingWorld.linearVelocity(slidingSphere);
    const glm::vec3 rollingAngularVelocity =
        slidingWorld.angularVelocity(slidingSphere);
    const glm::vec3 rollingContactArm(0.0f, -0.5f, 0.0f);
    const glm::vec3 rollingContactVelocity = rollingLinearVelocity +
        glm::cross(rollingAngularVelocity, rollingContactArm);
    passed &= expect(
        slidingSphere.position().x < frictionlessSphere.position().x * 0.85f,
        "ground friction reduces the travel distance compared with a frictionless sphere");
    passed &= expect(
        std::fabs(rollingContactVelocity.x) < 0.05f &&
            std::fabs(rollingContactVelocity.z) < 0.05f,
        "ground friction transitions a sliding sphere into rolling without slipping");
    passed &= expect(
        std::fabs(rollingAngularVelocity.z) > 1.0f,
        "ground friction generates the angular velocity required for rolling");

    const glm::quat rollingOrientation = slidingSphere.orientation();
    passed &= expect(
        std::fabs(glm::dot(rollingOrientation, rollingOrientation) - 1.0f) < 1e-4f,
        "integrated orientation remains a normalized quaternion");
    slidingWorld.reset();
    passed &= expect(
        glm::length(slidingWorld.angularVelocity(slidingSphere)) < 1e-6f &&
            glm::length(glm::vec3(slidingSphere.orientation().x,
                                  slidingSphere.orientation().y,
                                  slidingSphere.orientation().z)) < 1e-6f,
        "reset restores the initial angular velocity and orientation");

    PhysicsWorld staticFrictionWorld;
    staticFrictionWorld.setGroundPlane(0.0f, 0.8f, 0.6f);
    Transform barelySlidingSphere;
    barelySlidingSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    staticFrictionWorld.addSphereBody(
        barelySlidingSphere,
        glm::vec3(0.02f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.8f,
        0.6f);
    for (int i = 0; i < 30; ++i)
        staticFrictionWorld.update(1.0f / 120.0f);
    passed &= expect(
        barelySlidingSphere.position().x < 0.005f,
        "static ground friction prevents small persistent sliding");

    PhysicsWorld overlapWorld;
    overlapWorld.setGravity(glm::vec3(0.0f));
    Transform overlappingA;
    Transform overlappingB;
    overlapWorld.addSphereBody(overlappingA, glm::vec3(0.0f), 0.5f);
    overlapWorld.addSphereBody(overlappingB, glm::vec3(0.0f), 0.5f);
    overlapWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::isfinite(glm::length(overlappingB.position() - overlappingA.position())) &&
            glm::length(overlappingB.position() - overlappingA.position()) > 0.99f,
        "coincident spheres are separated without producing invalid positions");

    PhysicsWorld angularImpulseWorld;
    angularImpulseWorld.setGravity(glm::vec3(0.0f));
    Transform angularImpulseBody;
    angularImpulseBody.move(glm::vec3(0.0f, 1.0f, 0.0f));
    angularImpulseWorld.addSphereBody(
        angularImpulseBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f);
    const glm::vec3 positionBeforeAngularImpulse = angularImpulseBody.position();
    passed &= expect(
        angularImpulseWorld.applyAngularImpulse(
            angularImpulseBody, glm::vec3(0.0f, 0.8f, 0.0f)),
        "angular impulse is accepted for a dynamic sphere");
    passed &= expect(
        glm::length(angularImpulseWorld.linearVelocity(angularImpulseBody)) < 1e-6f &&
            angularImpulseWorld.angularVelocity(angularImpulseBody).y > 0.0f,
        "angular impulse changes spin without changing linear velocity");
    angularImpulseWorld.update(1.0f / 120.0f);
    passed &= expect(
        glm::length(angularImpulseBody.position() - positionBeforeAngularImpulse) < 1e-6f &&
            std::fabs(glm::dot(angularImpulseBody.orientation(),
                               glm::quat(1.0f, 0.0f, 0.0f, 0.0f))) < 0.99999f,
        "angular impulse rotates the body in place");

    PhysicsWorld dampingWorld;
    dampingWorld.setGravity(glm::vec3(0.0f));
    Transform dampingBody;
    dampingWorld.addSphereBody(
        dampingBody,
        glm::vec3(10.0f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        glm::vec3(0.0f, 5.0f, 0.0f));
    passed &= expect(
        dampingWorld.setLinearDamping(dampingBody, 2.0f) &&
            dampingWorld.setAngularDamping(dampingBody, 3.0f),
        "dynamic bodies accept linear and angular damping");
    dampingWorld.update(1.0f / 120.0f);
    const float substep = 1.0f / 120.0f;
    passed &= expect(
        std::fabs(dampingWorld.linearVelocity(dampingBody).x -
                  10.0f * std::exp(-2.0f * substep)) < 1e-5f &&
            std::fabs(dampingWorld.angularVelocity(dampingBody).y -
                      5.0f * std::exp(-3.0f * substep)) < 1e-5f,
        "linear and angular damping use frame-rate-independent exponential decay");

    PhysicsWorld timeScaleWorld;
    timeScaleWorld.setGravity(glm::vec3(0.0f));
    Transform timeScaleBody;
    timeScaleWorld.addSphereBody(
        timeScaleBody, glm::vec3(10.0f, 0.0f, 0.0f), 0.5f);
    timeScaleWorld.setTimeScale(0.5f);
    timeScaleWorld.update(0.1f);
    passed &= expect(
        timeScaleWorld.timeScale() == 0.5f &&
            timeScaleBody.position().x > 0.49f &&
            timeScaleBody.position().x < 0.51f,
        "physics time scale applies a smooth slow-motion factor");

    PhysicsWorld axisForceWorld;
    axisForceWorld.setGravity(glm::vec3(0.0f));
    Transform axisForceBody;
    axisForceWorld.addSphereBody(axisForceBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    axisForceWorld.applyForce(axisForceBody, glm::vec3(0.0f, 0.0f, 12.0f));
    axisForceWorld.update(1.0f / 120.0f);
    const glm::vec3 forceVelocity = axisForceWorld.linearVelocity(axisForceBody);
    passed &= expect(
        forceVelocity.z > 0.0f && std::fabs(forceVelocity.x) < 1e-6f &&
            std::fabs(forceVelocity.y) < 1e-6f,
        "a force on the Z axis produces translation along Z only");

    PhysicsWorld showcaseWorld;
    showcaseWorld.setGroundPlane(0.0f, 0.95f, 0.78f);
    Transform orangeShowcaseSphere;
    Transform cyanShowcaseSphere;
    orangeShowcaseSphere.move(glm::vec3(-2.7f, 0.5f, 0.0f));
    cyanShowcaseSphere.move(glm::vec3(2.7f, 0.5f, 0.0f));
    showcaseWorld.addSphereBody(
        orangeShowcaseSphere, glm::vec3(3.2f, 0.0f, 0.0f),
        0.5f, 0.35f, 1.0f, 0.95f, 0.78f);
    showcaseWorld.addSphereBody(
        cyanShowcaseSphere, glm::vec3(-2.4f, 0.0f, 0.0f),
        0.5f, 0.35f, 2.0f, 0.95f, 0.78f);
    for (int i = 0; i < 180; ++i)
        showcaseWorld.update(1.0f / 120.0f);

    const float showcaseDistance = glm::length(
        cyanShowcaseSphere.position() - orangeShowcaseSphere.position());
    passed &= expect(
        std::fabs(orangeShowcaseSphere.position().y - 0.5f) < 1e-4f &&
            std::fabs(cyanShowcaseSphere.position().y - 0.5f) < 1e-4f,
        "Dynamis rolling demonstration keeps both spheres supported by the floor");
    passed &= expect(
        showcaseDistance >= 0.999f,
        "Dynamis rolling demonstration resolves the head-on sphere collision");
    passed &= expect(
        std::fabs(showcaseWorld.angularVelocity(orangeShowcaseSphere).z) > 0.5f &&
            std::fabs(showcaseWorld.angularVelocity(cyanShowcaseSphere).z) > 0.5f,
        "both spheres acquire visible spin during the rolling collision demonstration");

    Transform orientationTransform;
    orientationTransform.setRotation(glm::vec3(25.0f, 40.0f, 15.0f));
    const glm::mat4 originalOrientationMatrix = orientationTransform.matrix();
    orientationTransform.setOrientation(orientationTransform.orientation());
    const glm::mat4 restoredOrientationMatrix = orientationTransform.matrix();
    float orientationMatrixError = 0.0f;
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            orientationMatrixError += std::fabs(
                originalOrientationMatrix[column][row] -
                restoredOrientationMatrix[column][row]);
        }
    }
    passed &= expect(
        orientationMatrixError < 1e-4f,
        "quaternion orientation preserves the existing Y-X-Z Euler convention");

    // O controle WASD aplica a mesma força aos corpos selecionados. A massa
    // determina a aceleração, enquanto o contato com o piso produz o rolamento.
    PhysicsWorld keyboardControlWorld;
    keyboardControlWorld.setGroundPlane(0.0f, 0.95f, 0.78f);
    Transform lightControlledSphere;
    Transform heavyControlledSphere;
    lightControlledSphere.move(glm::vec3(-2.0f, 0.5f, 0.0f));
    heavyControlledSphere.move(glm::vec3(2.0f, 0.5f, 0.0f));
    keyboardControlWorld.addSphereBody(
        lightControlledSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    keyboardControlWorld.addSphereBody(
        heavyControlledSphere, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f, 0.95f, 0.78f);
    for (int i = 0; i < 120; ++i)
    {
        keyboardControlWorld.applyForce(
            lightControlledSphere, glm::vec3(12.0f, 0.0f, 0.0f));
        keyboardControlWorld.applyForce(
            heavyControlledSphere, glm::vec3(12.0f, 0.0f, 0.0f));
        keyboardControlWorld.update(1.0f / 120.0f);
    }
    const float lightControlSpeed =
        keyboardControlWorld.linearVelocity(lightControlledSphere).x;
    const float heavyControlSpeed =
        keyboardControlWorld.linearVelocity(heavyControlledSphere).x;
    passed &= expect(
        lightControlledSphere.position().x > -2.0f &&
            heavyControlledSphere.position().x > 2.0f &&
            lightControlSpeed > 0.0f && heavyControlSpeed > 0.0f,
        "continuous WASD-style force moves a selected body without teleporting it");
    passed &= expect(
        lightControlSpeed > heavyControlSpeed * 1.9f &&
            lightControlSpeed < heavyControlSpeed * 2.1f,
        "the same WASD force accelerates a 1 kg sphere more than a 2 kg sphere");
    passed &= expect(
        keyboardControlWorld.angularVelocity(lightControlledSphere).z < 0.0f &&
            keyboardControlWorld.angularVelocity(heavyControlledSphere).z < 0.0f,
        "WASD force at sphere center produces natural rolling through ground friction");

    PhysicsWorld bodyQueryWorld;
    Transform queriedDynamicBody;
    Transform queriedStaticBody;
    Transform queriedUnregisteredBody;
    bodyQueryWorld.addSphereBody(
        queriedDynamicBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.5f);
    bodyQueryWorld.addStaticSphere(queriedStaticBody, 0.5f);
    passed &= expect(
        bodyQueryWorld.hasDynamicBody(queriedDynamicBody) &&
            !bodyQueryWorld.hasDynamicBody(queriedStaticBody) &&
            !bodyQueryWorld.hasDynamicBody(queriedUnregisteredBody),
        "dynamic body lookup distinguishes dynamic, static, and unregistered transforms");
    PhysicsWorld invalidMassWorld;
    Transform invalidMassBody;
    passed &= expect(
        !invalidMassWorld.addSphereBody(
            invalidMassBody, glm::vec3(0.0f), 0.5f, 0.0f, 0.0f) &&
            invalidMassWorld.bodyCount() == 0,
        "zero mass is rejected instead of creating an invalid dynamic body");


    // Força contínua: F = m*a, e a força é consumida após os subpassos do update.
    PhysicsWorld forceWorld;
    forceWorld.setGravity(glm::vec3(0.0f));
    Transform forceBody;
    forceWorld.addSphereBody(forceBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f);
    passed &= expect(
        forceWorld.applyForce(forceBody, glm::vec3(4.0f, 0.0f, 0.0f)),
        "registered dynamic body accepts an external force");
    forceWorld.update(2.0f / 120.0f);
    const float velocityAfterForce = forceWorld.linearVelocity(forceBody).x;
    passed &= expect(
        std::fabs(velocityAfterForce - (4.0f / 2.0f) * (2.0f / 120.0f)) < 1e-5f,
        "external force produces acceleration according to mass across all substeps");
    forceWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(forceWorld.linearVelocity(forceBody).x - velocityAfterForce) < 1e-6f,
        "accumulated force is cleared after the update consumes it");

    // Uma força aplicada antes de haver um passo fixo permanece pendente.
    PhysicsWorld deferredForceWorld;
    deferredForceWorld.setGravity(glm::vec3(0.0f));
    Transform deferredForceBody;
    deferredForceWorld.addSphereBody(deferredForceBody, glm::vec3(0.0f), 0.5f, 0.0f);
    deferredForceWorld.applyForce(deferredForceBody, glm::vec3(6.0f, 0.0f, 0.0f));
    deferredForceWorld.update(1.0f / 240.0f);
    passed &= expect(
        glm::length(deferredForceWorld.linearVelocity(deferredForceBody)) < 1e-6f,
        "force waits when the accumulator has not reached a fixed step");
    deferredForceWorld.update(1.0f / 240.0f);
    passed &= expect(
        std::fabs(deferredForceWorld.linearVelocity(deferredForceBody).x - 6.0f / 120.0f) < 1e-5f,
        "deferred force is applied when the next fixed step becomes available");

    // Força contínua é definida uma vez por frame e lida em cada passo fixo.
    // O resultado deve independer da frequência de renderização.
    const auto simulateContinuousForce = [](int renderFps)
    {
        PhysicsWorld world;
        world.setGravity(glm::vec3(0.0f));
        Transform body;
        world.addSphereBody(body, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);

        const float frameTime = 1.0f / static_cast<float>(renderFps);
        for (int frame = 0; frame < renderFps; ++frame)
        {
            world.setContinuousForce(body, glm::vec3(12.0f, 0.0f, 0.0f));
            world.update(frameTime);
        }
        return world.linearVelocity(body).x;
    };
    const float forceAt60Fps = simulateContinuousForce(60);
    const float forceAt144Fps = simulateContinuousForce(144);
    const float forceAt240Fps = simulateContinuousForce(240);
    passed &= expect(
        std::fabs(forceAt60Fps - 12.0f) < 0.03f &&
            std::fabs(forceAt144Fps - 12.0f) < 0.03f &&
            std::fabs(forceAt240Fps - 12.0f) < 0.03f,
        "continuous force produces the same acceleration at 60, 144, and 240 render FPS");

    PhysicsWorld releasedForceWorld;
    releasedForceWorld.setGravity(glm::vec3(0.0f));
    Transform releasedForceBody;
    releasedForceWorld.addSphereBody(releasedForceBody, glm::vec3(0.0f), 0.5f, 0.0f);
    releasedForceWorld.setContinuousForce(
        releasedForceBody, glm::vec3(10.0f, 0.0f, 0.0f));
    releasedForceWorld.update(1.0f / 120.0f);
    const float velocityBeforeRelease =
        releasedForceWorld.linearVelocity(releasedForceBody).x;
    releasedForceWorld.setContinuousForce(releasedForceBody, glm::vec3(0.0f));
    releasedForceWorld.update(1.0f / 120.0f);
    passed &= expect(
        velocityBeforeRelease > 0.0f &&
            std::fabs(releasedForceWorld.linearVelocity(releasedForceBody).x -
                      velocityBeforeRelease) < 1e-6f,
        "releasing a continuous force stops acceleration but preserves inertia");

    // O colisor esférico segue a maior escala absoluta do Transform.
    PhysicsWorld scaledColliderWorld;
    scaledColliderWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    scaledColliderWorld.setGroundPlane(0.0f);
    Transform scaledColliderBody;
    scaledColliderBody.move(glm::vec3(0.0f, 0.5f, 0.0f));
    scaledColliderWorld.addSphereBody(
        scaledColliderBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    scaledColliderBody.setScale(glm::vec3(1.0f, 2.0f, 0.75f));
    scaledColliderWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(scaledColliderBody.position().y - 1.0f) < 2e-4f,
        "sphere collider expands with the largest absolute Transform scale component");

    PhysicsWorld scaledInertiaWorld;
    scaledInertiaWorld.setGravity(glm::vec3(0.0f));
    Transform scaledInertiaBody;
    scaledInertiaWorld.addSphereBody(
        scaledInertiaBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    scaledInertiaBody.setScale(glm::vec3(2.0f));
    scaledInertiaWorld.applyAngularImpulse(
        scaledInertiaBody, glm::vec3(0.0f, 0.0f, 1.0f));
    passed &= expect(
        std::fabs(scaledInertiaWorld.angularVelocity(scaledInertiaBody).z - 2.5f) < 1e-4f,
        "sphere moment of inertia is recalculated after its Transform scale changes");

    // Subpassos adaptativos impedem que uma esfera rápida atravesse outro corpo.
    PhysicsWorld fastCollisionWorld;
    fastCollisionWorld.setGravity(glm::vec3(0.0f));
    Transform fastStaticSphere;
    Transform fastMovingSphere;
    fastStaticSphere.move(glm::vec3(0.0f));
    fastMovingSphere.move(glm::vec3(-2.0f, 0.0f, 0.0f));
    fastCollisionWorld.addStaticSphere(
        fastStaticSphere, 0.5f, 1.0f, 0.0f, 0.0f);
    fastCollisionWorld.addSphereBody(
        fastMovingSphere, glm::vec3(300.0f, 0.0f, 0.0f),
        0.5f, 1.0f, 1.0f, 0.0f, 0.0f);
    fastCollisionWorld.update(1.0f / 120.0f);
    passed &= expect(
        fastCollisionWorld.linearVelocity(fastMovingSphere).x < 0.0f &&
            fastMovingSphere.position().x < -1.0f,
        "adaptive substeps prevent a high-speed sphere from tunneling through a static sphere");

    const auto simulateContinuousTorque = [](int renderFps)
    {
        PhysicsWorld world;
        world.setGravity(glm::vec3(0.0f));
        Transform body;
        world.addSphereBody(body, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);

        const float frameTime = 1.0f / static_cast<float>(renderFps);
        for (int frame = 0; frame < renderFps; ++frame)
        {
            world.setContinuousTorque(body, glm::vec3(0.0f, 0.0f, 0.1f));
            world.update(frameTime);
        }
        return world.angularVelocity(body).z;
    };
    const float torqueAt60Fps = simulateContinuousTorque(60);
    const float torqueAt240Fps = simulateContinuousTorque(240);
    passed &= expect(
        std::fabs(torqueAt60Fps - 1.0f) < 0.01f &&
            std::fabs(torqueAt240Fps - 1.0f) < 0.01f,
        "continuous torque produces the same angular acceleration at different render FPS");

    PhysicsWorld torqueWorld;
    torqueWorld.setGravity(glm::vec3(0.0f));
    Transform torqueBody;
    torqueWorld.addSphereBody(torqueBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    passed &= expect(
        torqueWorld.applyTorque(torqueBody, glm::vec3(0.0f, 0.0f, 1.0f)),
        "dynamic sphere accepts external torque");
    torqueWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(torqueWorld.angularVelocity(torqueBody).z - (1.0f / 0.1f) / 120.0f) < 1e-5f,
        "torque produces angular acceleration using the sphere moment of inertia");
    passed &= expect(
        glm::length(torqueWorld.linearVelocity(torqueBody)) < 1e-6f,
        "pure torque does not introduce linear velocity");

    PhysicsWorld impulseWorld;
    impulseWorld.setGravity(glm::vec3(0.0f));
    Transform impulseBody;
    impulseWorld.addSphereBody(impulseBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f);
    passed &= expect(
        impulseWorld.applyImpulseAtPoint(
            impulseBody,
            glm::vec3(2.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.25f, 0.0f)),
        "off-center impulse is accepted");
    passed &= expect(
        std::fabs(impulseWorld.linearVelocity(impulseBody).x - 1.0f) < 1e-5f &&
            std::fabs(impulseWorld.angularVelocity(impulseBody).z + 2.5f) < 1e-5f,
        "off-center impulse changes both linear and angular velocity immediately");

    PhysicsWorld forceAtPointWorld;
    forceAtPointWorld.setGravity(glm::vec3(0.0f));
    Transform forceAtPointBody;
    forceAtPointWorld.addSphereBody(forceAtPointBody, glm::vec3(0.0f), 0.5f, 0.0f);
    passed &= expect(
        forceAtPointWorld.applyForceAtPoint(
            forceAtPointBody,
            glm::vec3(4.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.25f, 0.0f)),
        "force applied at a world-space point is accepted");
    forceAtPointWorld.update(1.0f / 120.0f);
    passed &= expect(
        forceAtPointWorld.linearVelocity(forceAtPointBody).x > 0.03f &&
            forceAtPointWorld.angularVelocity(forceAtPointBody).z < -0.08f,
        "off-center force generates both translation and torque");

    PhysicsWorld guardedForceWorld;
    Transform staticForceBody;
    guardedForceWorld.addStaticSphere(staticForceBody, 0.5f);
    passed &= expect(
        !guardedForceWorld.applyForce(staticForceBody, glm::vec3(10.0f, 0.0f, 0.0f)) &&
            !guardedForceWorld.applyImpulse(staticForceBody, glm::vec3(10.0f, 0.0f, 0.0f)) &&
            !guardedForceWorld.applyTorque(staticForceBody, glm::vec3(0.0f, 0.0f, 10.0f)),
        "static bodies reject external force, impulse, and torque");
    Transform unregisteredBody;
    passed &= expect(
        !guardedForceWorld.applyForce(unregisteredBody, glm::vec3(1.0f, 0.0f, 0.0f)),
        "unregistered transforms reject external forces");
    passed &= expect(
        !guardedForceWorld.applyImpulse(
            staticForceBody,
            glm::vec3(std::numeric_limits<float>::infinity(), 0.0f, 0.0f)),
        "non-finite impulses are rejected");

    PhysicsWorld fastGroundWorld;
    fastGroundWorld.setGravity(glm::vec3(0.0f));
    fastGroundWorld.setGroundPlane(0.0f, 0.0f, 0.0f);
    Transform fastFallingSphere;
    fastFallingSphere.move(glm::vec3(0.0f, 2.0f, 0.0f));
    fastGroundWorld.addSphereBody(
        fastFallingSphere, glm::vec3(0.0f, -300.0f, 0.0f),
        0.5f, 1.0f, 1.0f, 0.0f, 0.0f);
    fastGroundWorld.update(1.0f / 120.0f);
    passed &= expect(
        fastFallingSphere.position().y > 0.5f &&
            fastGroundWorld.linearVelocity(fastFallingSphere).y > 0.0f,
        "adaptive substeps prevent a high-speed sphere from tunneling through the ground plane");

    // O tampo da mesa é uma caixa estática independente dos corpos dinâmicos.
    PhysicsWorld tabletopWorld;
    Transform tabletopTransform;
    passed &= expect(
        tabletopWorld.addStaticBox(
            tabletopTransform,
            glm::vec3(0.0f),
            glm::vec3(1.0f, 0.1f, 0.75f),
            0.0f,
            0.9f,
            0.7f),
        "static oriented box collider accepts valid dimensions");
    Transform tabletopSphere;
    tabletopSphere.move(glm::vec3(0.0f, 1.0f, 0.0f));
    passed &= expect(
        tabletopWorld.addSphereBody(
            tabletopSphere, glm::vec3(0.0f), 0.25f, 0.0f, 1.0f),
        "dynamic sphere is accepted above a static box collider");
    passed &= expect(
        tabletopWorld.bodyCount() == 1,
        "static box collider is not counted as a dynamic body");
    for (int i = 0; i < 240; ++i)
        tabletopWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(tabletopSphere.position().y - 0.35f) < 2e-3f,
        "sphere rests on top of a static box instead of passing through it");

    // Fora da projeção horizontal do tampo, a esfera não deve colidir com ele.
    PhysicsWorld outsideTableWorld;
    Transform outsideTable;
    outsideTableWorld.addStaticBox(
        outsideTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 0.75f));
    Transform outsideTableSphere;
    outsideTableSphere.move(glm::vec3(1.6f, 1.0f, 0.0f));
    outsideTableWorld.addSphereBody(
        outsideTableSphere, glm::vec3(0.0f), 0.25f, 0.0f, 1.0f);
    for (int i = 0; i < 120; ++i)
        outsideTableWorld.update(1.0f / 120.0f);
    passed &= expect(
        outsideTableSphere.position().y < 0.0f,
        "static box only collides within its finite footprint");

    // Mesmo em uma queda muito rápida, subpassos devem detectar o tampo.
    PhysicsWorld fastTabletopWorld;
    fastTabletopWorld.setGravity(glm::vec3(0.0f));
    Transform fastTable;
    fastTabletopWorld.addStaticBox(
        fastTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 0.75f),
        0.0f, 0.0f, 0.0f);
    Transform fastTableSphere;
    fastTableSphere.move(glm::vec3(0.0f, 2.0f, 0.0f));
    fastTabletopWorld.addSphereBody(
        fastTableSphere, glm::vec3(0.0f, -300.0f, 0.0f),
        0.25f, 0.0f, 1.0f, 0.0f, 0.0f);
    fastTabletopWorld.update(1.0f / 120.0f);
    passed &= expect(
        fastTableSphere.position().y > 0.34f &&
            fastTabletopWorld.linearVelocity(fastTableSphere).y > -1e-3f,
        "adaptive substeps prevent a fast sphere from tunneling through a static box");

    // Caixas também precisam participar do cálculo de subpassos adaptativos.
    // Sem isso, o corpo OBB atravessa a plataforma no único passo fixo.
    PhysicsWorld fastBoxWorld;
    fastBoxWorld.setGravity(glm::vec3(0.0f));
    Transform thinStaticWall;
    fastBoxWorld.addStaticBox(
        thinStaticWall, glm::vec3(0.0f), glm::vec3(0.04f, 0.5f, 0.5f),
        0.0f, 0.0f, 0.0f);
    Transform fastDynamicBox;
    fastDynamicBox.move(glm::vec3(-1.0f, 0.0f, 0.0f));
    passed &= expect(
        fastBoxWorld.addBoxBody(
            fastDynamicBox, glm::vec3(0.04f, 0.1f, 0.1f),
            glm::vec3(300.0f, 0.0f, 0.0f), 0.0f, 1.0f, 0.0f, 0.0f),
        "fast dynamic OBB is registered");
    fastBoxWorld.update(1.0f / 120.0f);
    passed &= expect(
        fastDynamicBox.position().x < -0.079f &&
            fastBoxWorld.linearVelocity(fastDynamicBox).x < 1e-3f,
        "adaptive substeps prevent a high-speed OBB from tunneling through a static wall");

    // O colisor acompanha a transformação da mesa, inclusive escala e rotação.
    PhysicsWorld transformedTableWorld;
    Transform transformedTable;
    transformedTable.move(glm::vec3(2.0f, 0.0f, -1.0f));
    transformedTable.setRotation(glm::vec3(0.0f, 45.0f, 0.0f));
    transformedTable.setScale(glm::vec3(2.0f, 1.0f, 0.5f));
    transformedTableWorld.addStaticBox(
        transformedTable,
        glm::vec3(0.0f, 0.2f, 0.0f),
        glm::vec3(1.0f, 0.1f, 0.75f));
    Transform transformedTableSphere;
    transformedTableSphere.move(glm::vec3(2.0f, 1.0f, -1.0f));
    transformedTableWorld.addSphereBody(
        transformedTableSphere, glm::vec3(0.0f), 0.25f, 0.0f, 1.0f);
    for (int i = 0; i < 180; ++i)
        transformedTableWorld.update(1.0f / 120.0f);
    passed &= expect(
        transformedTableSphere.position().y > 0.45f,
        "static box follows its Transform position, rotation, and scale");

    std::vector<Vertex> bowlVertices;
    std::vector<unsigned int> bowlIndices;
    passed &= expect(
        shapes::bowl(10.0f, 4.0f, 1.0f, 16, 32,
                     bowlVertices, bowlIndices),
        "procedural bowl geometry accepts valid dimensions and subdivisions");
    passed &= expect(
        bowlVertices.size() == 1 + 16 * 33 &&
            bowlIndices.size() == 32 * 3 + 15 * 32 * 6,
        "bowl geometry creates a single center vertex and indexed radial rings");
    passed &= expect(
        std::fabs(bowlVertices.front().position.y) < 1e-6f &&
            std::fabs(bowlVertices.back().position.y - 5.0f) < 1e-4f,
        "bowl profile reaches the requested height at its outer rim");
    bool bowlNormalsValid = true;
    for (const Vertex& vertex : bowlVertices)
    {
        const float normalLength = glm::length(vertex.normal);
        if (!std::isfinite(normalLength) ||
            std::fabs(normalLength - 1.0f) > 1e-4f ||
            vertex.normal.y <= 0.0f)
        {
            bowlNormalsValid = false;
            break;
        }
    }
    passed &= expect(
        bowlNormalsValid,
        "bowl vertices have normalized upward-facing surface normals");
    bool bowlWindingValid = true;
    for (std::size_t index = 0; index + 2 < bowlIndices.size(); index += 3)
    {
        const Vertex& a = bowlVertices[bowlIndices[index + 0]];
        const Vertex& b = bowlVertices[bowlIndices[index + 1]];
        const Vertex& c = bowlVertices[bowlIndices[index + 2]];
        const glm::vec3 faceNormal = glm::cross(
            b.position - a.position, c.position - a.position);
        const glm::vec3 averageNormal = a.normal + b.normal + c.normal;
        if (glm::dot(faceNormal, averageNormal) <= 0.0f)
        {
            bowlWindingValid = false;
            break;
        }
    }
    passed &= expect(
        bowlWindingValid,
        "bowl triangles face the inside of the basin consistently with their normals");
    passed &= expect(
        !shapes::bowl(0.0f, 4.0f, 1.0f, 16, 32,
                      bowlVertices, bowlIndices),
        "bowl geometry rejects invalid dimensions");

    PhysicsWorld bowlWorld;
    bowlWorld.setGroundBowl(10.0f, 4.0f, 1.0f, 0.95f, 0.78f);
    Transform bowlCenterSphere;
    bowlCenterSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    bowlWorld.addSphereBody(
        bowlCenterSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 120; ++i)
        bowlWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(bowlCenterSphere.position().y - 0.5f) < 1e-4f &&
            std::fabs(bowlCenterSphere.position().x) < 1e-4f &&
            std::fabs(bowlCenterSphere.position().z) < 1e-4f,
        "sphere remains at rest at the lowest point of the bowl");

    PhysicsWorld longTermBowlWorld;
    longTermBowlWorld.setGroundBowl(10.0f, 4.0f, 1.0f, 0.95f, 0.78f);
    Transform longTermRestingSphere;
    longTermRestingSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    longTermBowlWorld.addSphereBody(
        longTermRestingSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 1200; ++i)
        longTermBowlWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(longTermRestingSphere.position().y - 0.5f) < 1e-4f &&
            glm::length(longTermBowlWorld.linearVelocity(longTermRestingSphere)) < 1e-5f &&
            glm::length(longTermBowlWorld.angularVelocity(longTermRestingSphere)) < 1e-5f,
        "a resting sphere remains stable at the bowl bottom over ten simulated seconds");

    PhysicsWorld bowlRollingWorld;
    bowlRollingWorld.setGroundBowl(10.0f, 4.0f, 1.0f, 0.95f, 0.78f);
    Transform bowlSideSphere;
    // Centro da esfera = ponto da superfície (r=5) + normal * raio.
    const float bowlSlope = 0.4f;
    const glm::vec3 bowlNormal = glm::normalize(
        glm::vec3(-bowlSlope, 1.0f, 0.0f));
    const glm::vec3 bowlSurfacePoint(5.0f, 1.0f, 0.0f);
    bowlSideSphere.move(bowlSurfacePoint + bowlNormal * 0.5f);
    bowlRollingWorld.addSphereBody(
        bowlSideSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 60; ++i)
        bowlRollingWorld.update(1.0f / 120.0f);
    passed &= expect(
        bowlSideSphere.position().x < bowlSurfacePoint.x - 0.05f,
        "gravity accelerates a sphere down the inclined bowl toward its center");
    passed &= expect(
        glm::length(bowlRollingWorld.angularVelocity(bowlSideSphere)) > 0.1f,
        "friction against the inclined bowl creates rolling angular velocity");

    PhysicsWorld longRunBowlWorld;
    longRunBowlWorld.setGroundBowl(18.0f, 4.5f, 2.5f, 0.95f, 0.78f);
    Transform longRunSlopeSphere;
    const float initialBowlSlope = 2.0f * 4.5f * 5.0f / (18.0f * 18.0f);
    const glm::vec3 initialBowlNormal = glm::normalize(
        glm::vec3(-initialBowlSlope, 1.0f, 0.0f));
    const glm::vec3 initialBowlSurfacePoint(
        5.0f, 4.5f * 25.0f / (18.0f * 18.0f), 0.0f);
    longRunSlopeSphere.move(initialBowlSurfacePoint + initialBowlNormal * 0.5f);
    longRunBowlWorld.addSphereBody(
        longRunSlopeSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 1200; ++i)
        longRunBowlWorld.update(1.0f / 120.0f);
    const glm::vec3 longRunPosition = longRunSlopeSphere.position();
    const float longRunRadialDistance = std::sqrt(
        longRunPosition.x * longRunPosition.x +
        longRunPosition.z * longRunPosition.z);
    const bool longRunFinite =
        std::isfinite(longRunPosition.x) && std::isfinite(longRunPosition.y) &&
        std::isfinite(longRunPosition.z) &&
        std::isfinite(glm::length(longRunBowlWorld.linearVelocity(longRunSlopeSphere))) &&
        std::isfinite(glm::length(longRunBowlWorld.angularVelocity(longRunSlopeSphere)));
    passed &= expect(
        longRunFinite && longRunRadialDistance < 18.0f &&
            longRunPosition.y > 0.0f,
        "a sphere rolling along the curved bowl remains finite and contained over ten seconds");

    // Caixas dinâmicas usam a dimensão real do volume, sem transformar
    // objetos planos em esferas enormes que os fariam flutuar sobre o chão.
    PhysicsWorld thinBoxWorld;
    thinBoxWorld.setGroundPlane(0.0f, 0.9f, 0.7f);
    Transform thinBox;
    thinBox.move(glm::vec3(0.0f, 0.5f, 0.0f));
    passed &= expect(
        thinBoxWorld.addBoxBody(
            thinBox, glm::vec3(0.5f, 0.01f, 0.3f),
            glm::vec3(0.0f), 0.0f, 1.0f, 0.9f, 0.7f),
        "dynamic OBB accepts valid local half extents");
    for (int i = 0; i < 240; ++i)
        thinBoxWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(thinBox.position().y - 0.01f) < 1e-4f &&
            glm::length(thinBoxWorld.angularVelocity(thinBox)) < 1e-4f,
        "thin box rests on the ground using its actual thickness, without floating");

    PhysicsWorld scaledBoxWorld;
    scaledBoxWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    scaledBoxWorld.setGroundPlane(0.0f);
    Transform scaledBox;
    scaledBox.setScale(glm::vec3(1.0f, 2.0f, 0.75f));
    scaledBox.move(glm::vec3(0.0f, 0.5f, 0.0f));
    passed &= expect(
        scaledBoxWorld.addBoxBody(
            scaledBox, glm::vec3(0.5f, 0.1f, 0.25f),
            glm::vec3(0.0f), 0.0f, 1.0f),
        "scaled OBB body is registered");
    for (int i = 0; i < 240; ++i)
        scaledBoxWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(scaledBox.position().y - 0.2f) < 2e-4f,
        "box collider tracks non-uniform Transform scale and uses its Y half extent");

    // A face-on-face collision must produce a patch, not a single support pivot.
    {
        using rotor::physics::collision::intersectOrientedBoxes;
        using rotor::physics::collision::makeOrientedBox;
        const auto tableBox = makeOrientedBox(
            glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.1f, 1.0f));
        const auto bookBox = makeOrientedBox(
            glm::vec3(0.0f, 0.12f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(0.3f, 0.02f, 0.3f));
        rotor::physics::ContactManifold manifold;
        const bool detected = intersectOrientedBoxes(tableBox, bookBox, manifold);
        passed &= expect(
            detected && manifold.pointCount == 4,
            "face-to-face OBB collision generates four contact points");
        for (std::size_t i = 0; i < manifold.pointCount; ++i)
        {
            passed &= expect(
                manifold.points[i].hasSeparatePoints,
                "each OBB contact stores one point on each collider surface");
        }
    }

    // A box resting flat on a plane should contact through all four lower vertices.
    {
        using rotor::physics::collision::boxPlaneContacts;
        using rotor::physics::collision::makeOrientedBox;
        const auto box = makeOrientedBox(
            glm::vec3(0.0f, 0.1f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(0.3f, 0.1f, 0.2f));
        rotor::physics::ContactManifold manifold;
        const bool detected = boxPlaneContacts(
            box, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, manifold);
        passed &= expect(
            detected && manifold.pointCount == 4,
            "flat box-plane contact generates a four-point support patch");

        const auto deeplyOverlappingBox = makeOrientedBox(
            glm::vec3(0.0f, -0.05f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(0.5f, 0.1f, 0.3f));
        rotor::physics::ContactManifold deepManifold;
        const bool deepDetected = boxPlaneContacts(
            deeplyOverlappingBox, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f,
            deepManifold);
        bool onlyLowestSupportFeature = deepDetected && deepManifold.pointCount == 4;
        for (std::size_t i = 0; i < deepManifold.pointCount; ++i)
        {
            onlyLowestSupportFeature = onlyLowestSupportFeature &&
                std::fabs(deepManifold.points[i].pointB.y + 0.15f) < 1e-5f &&
                std::fabs(deepManifold.points[i].penetration - 0.15f) < 1e-5f;
        }
        passed &= expect(
            onlyLowestSupportFeature,
            "deep box-plane overlap uses only the lowest support face");
    }

    PhysicsWorld obbTableWorld;
    obbTableWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    Transform tabletop;
    passed &= expect(
        obbTableWorld.addStaticBox(
            tabletop, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 1.0f)),
        "static table top OBB is registered");
    Transform fallingBook;
    fallingBook.move(glm::vec3(0.0f, 1.0f, 0.0f));
    passed &= expect(
        obbTableWorld.addBoxBody(
            fallingBook, glm::vec3(0.25f, 0.02f, 0.25f),
            glm::vec3(0.0f), 0.0f, 0.5f),
        "dynamic book-sized OBB is registered");
    for (int i = 0; i < 480; ++i)
        obbTableWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(fallingBook.position().y - 0.12f) < 1e-3f &&
            std::fabs(obbTableWorld.linearVelocity(fallingBook).y) < 1e-4f &&
            glm::length(obbTableWorld.angularVelocity(fallingBook)) < 1e-4f &&
            std::fabs(fallingBook.position().x) < 1e-4f &&
            std::fabs(fallingBook.position().z) < 1e-4f,
        "symmetric vertical box impact rests on the table without artificial spin or drift");

    PhysicsWorld boxPairWorld;
    boxPairWorld.setGravity(glm::vec3(0.0f));
    Transform movingBox;
    Transform targetBox;
    movingBox.move(glm::vec3(-0.4f, 0.0f, 0.0f));
    targetBox.move(glm::vec3(0.4f, 0.0f, 0.0f));
    passed &= expect(
        boxPairWorld.addBoxBody(
            movingBox, glm::vec3(0.5f, 0.2f, 0.2f),
            glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f) &&
        boxPairWorld.addBoxBody(
            targetBox, glm::vec3(0.5f, 0.2f, 0.2f),
            glm::vec3(0.0f), 0.0f, 1.0f),
        "two dynamic OBB bodies are registered");
    for (int i = 0; i < 120; ++i)
        boxPairWorld.update(1.0f / 120.0f);
    passed &= expect(
        targetBox.position().x - movingBox.position().x >= 0.998f &&
            std::fabs(boxPairWorld.linearVelocity(movingBox).x -
                      boxPairWorld.linearVelocity(targetBox).x) < 1e-3f &&
            glm::length(boxPairWorld.angularVelocity(movingBox)) < 1e-4f &&
            glm::length(boxPairWorld.angularVelocity(targetBox)) < 1e-4f,
        "symmetric face-to-face OBB impact transfers momentum without artificial spin");

    PhysicsWorld mixedShapeWorld;
    mixedShapeWorld.setGravity(glm::vec3(0.0f));
    Transform staticLikeBox;
    Transform mixedMovingSphere;
    mixedMovingSphere.move(glm::vec3(-0.6f, 0.0f, 0.0f));
    passed &= expect(
        mixedShapeWorld.addBoxBody(
            staticLikeBox, glm::vec3(0.5f), glm::vec3(0.0f),
            0.0f, 1.0f) &&
        mixedShapeWorld.addSphereBody(
            mixedMovingSphere, glm::vec3(1.0f, 0.0f, 0.0f),
            0.2f, 0.0f, 1.0f),
        "mixed sphere and box bodies are registered");
    for (int i = 0; i < 120; ++i)
        mixedShapeWorld.update(1.0f / 120.0f);
    passed &= expect(
        staticLikeBox.position().x > 0.1f &&
            staticLikeBox.position().x - mixedMovingSphere.position().x >= 0.698f,
        "sphere collides with a dynamic OBB instead of its bounding sphere");

    PhysicsWorld restingBoxOnTableWorld;
    restingBoxOnTableWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    Transform restingTable;
    restingBoxOnTableWorld.addStaticBox(
        restingTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 1.0f));
    Transform restingBoxOnTable;
    restingBoxOnTable.move(glm::vec3(0.0f, 0.12f, 0.0f));
    restingBoxOnTableWorld.addBoxBody(
        restingBoxOnTable, glm::vec3(0.3f, 0.02f, 0.3f),
        glm::vec3(0.0f), 0.0f, 1.0f);
    for (int i = 0; i < 1200; ++i)
        restingBoxOnTableWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(restingBoxOnTable.position().y - 0.12f) < 2e-4f &&
            glm::length(restingBoxOnTableWorld.linearVelocity(restingBoxOnTable)) < 1e-4f &&
            glm::length(restingBoxOnTableWorld.angularVelocity(restingBoxOnTable)) < 1e-4f,
        "box remains stable on the static tabletop over ten simulated seconds");

    PhysicsWorld offCenterBoxCollisionWorld;
    offCenterBoxCollisionWorld.setGravity(glm::vec3(0.0f));
    Transform offCenterMovingBox;
    Transform offCenterTargetBox;
    offCenterMovingBox.move(glm::vec3(-0.6f, 0.0f, 0.0f));
    offCenterTargetBox.move(glm::vec3(0.4f, 0.0f, 0.12f));
    offCenterBoxCollisionWorld.addBoxBody(
        offCenterMovingBox, glm::vec3(0.5f, 0.2f, 0.2f),
        glm::vec3(2.0f, 0.0f, 0.0f), 0.0f, 1.0f);
    offCenterBoxCollisionWorld.addBoxBody(
        offCenterTargetBox, glm::vec3(0.5f, 0.2f, 0.2f),
        glm::vec3(0.0f), 0.0f, 1.0f);
    for (int i = 0; i < 60; ++i)
        offCenterBoxCollisionWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(offCenterBoxCollisionWorld.angularVelocity(offCenterMovingBox).y) > 0.05f &&
            std::fabs(offCenterBoxCollisionWorld.angularVelocity(offCenterTargetBox).y) > 0.05f,
        "off-center OBB impact transfers angular motion to both boxes");

    PhysicsWorld stackedBoxesWorld;
    stackedBoxesWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    Transform stackedTable;
    stackedBoxesWorld.addStaticBox(
        stackedTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 1.0f));
    Transform lowerBox;
    Transform upperBox;
    lowerBox.move(glm::vec3(0.0f, 0.12f, 0.0f));
    upperBox.move(glm::vec3(0.015f, 0.16f, 0.0f));
    stackedBoxesWorld.addBoxBody(
        lowerBox, glm::vec3(0.3f, 0.02f, 0.3f), glm::vec3(0.0f), 0.0f, 1.0f);
    stackedBoxesWorld.addBoxBody(
        upperBox, glm::vec3(0.3f, 0.02f, 0.3f), glm::vec3(0.0f), 0.0f, 1.0f);
    float maximumLowerAngularSpeed = 0.0f;
    float maximumUpperAngularSpeed = 0.0f;
    for (int i = 0; i < 1200; ++i)
    {
        stackedBoxesWorld.update(1.0f / 120.0f);
        maximumLowerAngularSpeed = std::max(
            maximumLowerAngularSpeed,
            glm::length(stackedBoxesWorld.angularVelocity(lowerBox)));
        maximumUpperAngularSpeed = std::max(
            maximumUpperAngularSpeed,
            glm::length(stackedBoxesWorld.angularVelocity(upperBox)));
    }
    passed &= expect(
        std::fabs(lowerBox.position().y - 0.12f) < 0.001f &&
            std::fabs(upperBox.position().y - 0.16f) < 0.001f &&
            maximumLowerAngularSpeed < 0.002f &&
            maximumUpperAngularSpeed < 0.002f,
        "two offset boxes remain stable without artificial rocking on a static tabletop");

    PhysicsWorld threeBoxStackWorld;
    threeBoxStackWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    Transform threeBoxTable;
    threeBoxStackWorld.addStaticBox(
        threeBoxTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 1.0f));
    Transform bottomStackBox;
    Transform middleStackBox;
    Transform topStackBox;
    bottomStackBox.move(glm::vec3(0.0f, 0.12f, 0.0f));
    middleStackBox.move(glm::vec3(0.01f, 0.16f, 0.0f));
    topStackBox.move(glm::vec3(-0.01f, 0.20f, 0.0f));
    threeBoxStackWorld.addBoxBody(
        bottomStackBox, glm::vec3(0.3f, 0.02f, 0.3f),
        glm::vec3(0.0f), 0.0f, 1.0f);
    threeBoxStackWorld.addBoxBody(
        middleStackBox, glm::vec3(0.3f, 0.02f, 0.3f),
        glm::vec3(0.0f), 0.0f, 1.0f);
    threeBoxStackWorld.addBoxBody(
        topStackBox, glm::vec3(0.3f, 0.02f, 0.3f),
        glm::vec3(0.0f), 0.0f, 1.0f);

    glm::vec3 maximumStackAngularSpeed(0.0f);
    for (int i = 0; i < 2400; ++i)
    {
        threeBoxStackWorld.update(1.0f / 120.0f);
        maximumStackAngularSpeed.x = std::max(
            maximumStackAngularSpeed.x,
            glm::length(threeBoxStackWorld.angularVelocity(bottomStackBox)));
        maximumStackAngularSpeed.y = std::max(
            maximumStackAngularSpeed.y,
            glm::length(threeBoxStackWorld.angularVelocity(middleStackBox)));
        maximumStackAngularSpeed.z = std::max(
            maximumStackAngularSpeed.z,
            glm::length(threeBoxStackWorld.angularVelocity(topStackBox)));
    }
    passed &= expect(
        std::fabs(bottomStackBox.position().y - 0.12f) < 0.0015f &&
            std::fabs(middleStackBox.position().y - 0.16f) < 0.0015f &&
            std::fabs(topStackBox.position().y - 0.20f) < 0.0015f &&
            maximumStackAngularSpeed.x < 0.001f &&
            maximumStackAngularSpeed.y < 0.001f &&
            maximumStackAngularSpeed.z < 0.001f,
        "three slightly offset boxes remain stable as a supported stack");

    PhysicsWorld initialOverlapWorld;
    initialOverlapWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    Transform initialTable;
    initialOverlapWorld.addStaticBox(
        initialTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 1.0f));
    Transform initiallyPenetratingBox;
    initiallyPenetratingBox.move(glm::vec3(0.0f, 0.10f, 0.0f));
    initialOverlapWorld.addBoxBody(
        initiallyPenetratingBox, glm::vec3(0.25f, 0.02f, 0.25f),
        glm::vec3(0.0f), 0.0f, 1.0f);
    initialOverlapWorld.stabilizeInitialOverlaps(24);
    initialOverlapWorld.captureInitialPose();
    const float stabilizedInitialY = initiallyPenetratingBox.position().y;
    initialOverlapWorld.reset();
    passed &= expect(
        stabilizedInitialY >= 0.119f &&
            std::fabs(initiallyPenetratingBox.position().y - stabilizedInitialY) < 1e-6f &&
            glm::length(initialOverlapWorld.linearVelocity(initiallyPenetratingBox)) < 1e-6f &&
            glm::length(initialOverlapWorld.angularVelocity(initiallyPenetratingBox)) < 1e-6f,
        "initial overlap stabilization corrects pose without injecting motion and reset preserves it");

    PhysicsWorld scaledBoxInertiaCacheWorld;
    scaledBoxInertiaCacheWorld.setGravity(glm::vec3(0.0f));
    Transform scaledInertiaBox;
    passed &= expect(
        scaledBoxInertiaCacheWorld.addBoxBody(
            scaledInertiaBox, glm::vec3(1.0f, 0.5f, 0.25f)),
        "box for scale-dependent inertia cache test is registered");
    scaledInertiaBox.setScale(glm::vec3(2.0f, 1.0f, 1.0f));
    scaledBoxInertiaCacheWorld.applyAngularImpulse(
        scaledInertiaBox, glm::vec3(0.0f, 0.0f, 1.0f));
    passed &= expect(
        scaledBoxInertiaCacheWorld.angularVelocity(scaledInertiaBox).z > 0.6f &&
            scaledBoxInertiaCacheWorld.angularVelocity(scaledInertiaBox).z < 0.8f,
        "cached local inertia is rebuilt when the Transform scale changes");

    PhysicsWorld boxInertiaWorld;
    boxInertiaWorld.setGravity(glm::vec3(0.0f));
    Transform anisotropicBox;
    passed &= expect(
        boxInertiaWorld.addBoxBody(
            anisotropicBox, glm::vec3(1.0f, 0.1f, 0.5f),
            glm::vec3(0.0f), 0.0f, 1.0f),
        "anisotropic box is registered for rotational inertia test");
    boxInertiaWorld.applyAngularImpulse(
        anisotropicBox, glm::vec3(1.0f, 0.0f, 0.0f));
    const float angularVelocityX =
        boxInertiaWorld.angularVelocity(anisotropicBox).x;
    boxInertiaWorld.reset();
    boxInertiaWorld.applyAngularImpulse(
        anisotropicBox, glm::vec3(0.0f, 0.0f, 1.0f));
    const float angularVelocityZ =
        boxInertiaWorld.angularVelocity(anisotropicBox).z;
    passed &= expect(
        angularVelocityX > angularVelocityZ * 2.0f,
        "box inertia tensor responds differently around different local axes");

    // Malha de teste: caixa fechada com oito vértices e doze triângulos.
    const std::vector<glm::vec3> meshBoxVertices = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}};
    const std::vector<unsigned int> meshBoxIndices = {
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
        0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5,
        2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
    auto meshBoxData = std::make_shared<rotor::physics::TriangleMeshColliderData>();
    passed &= expect(
        rotor::physics::buildTriangleMeshColliderData(
            meshBoxVertices, meshBoxIndices, *meshBoxData),
        "triangle mesh collider builds from the source geometry");
    passed &= expect(
        meshBoxData->valid() && meshBoxData->vertices.size() == 8 &&
            meshBoxData->triangleIndices.size() == meshBoxIndices.size() &&
            glm::length(meshBoxData->localCenterOfMass) < 1e-5f &&
            glm::length(meshBoxData->localHalfExtents - glm::vec3(0.5f)) < 1e-5f,
        "triangle mesh deduplicates positions and computes centered local bounds");
    passed &= expect(
        !meshBoxData->bvhNodes.empty() &&
            meshBoxData->triangleOrder.size() == meshBoxIndices.size() / 3,
        "triangle mesh builds a reusable BVH over its triangle indices");

    Transform supportMeshTransform;
    rotor::physics::RigidBody supportMeshBody;
    supportMeshBody.transform = &supportMeshTransform;
    supportMeshBody.collider.shape = rotor::physics::ColliderShape::TriangleMesh;
    supportMeshBody.collider.triangleMesh = meshBoxData;
    rotor::physics::collision::MeshCollisionScratch supportScratch;
    supportScratch.collectStats = true;
    const glm::vec3 supportDirection(0.0f, 1.0f, 0.0f);
    const glm::vec3 exactSupportPoint =
        rotor::physics::collision::supportPoint(
            supportMeshBody, supportDirection, &supportScratch);
    float bruteForceSupport = std::numeric_limits<float>::lowest();
    for (const glm::vec3& vertex : meshBoxData->vertices)
        bruteForceSupport = std::max(bruteForceSupport, vertex.y);
    passed &= expect(
        std::fabs(exactSupportPoint.y - bruteForceSupport) < 1e-6f &&
            supportScratch.counters.supportBvhNodesVisited > 0 &&
            supportScratch.counters.supportVerticesTested <=
                meshBoxData->vertices.size() &&
            supportScratch.counters.supportVerticesTested <
                meshBoxData->triangleIndices.size(),
        "mesh support search uses the BVH and preserves the exact extreme");

    // Malha densa: a consulta de suporte deve podar a maior parte da BVH,
    // em vez de testar linearmente todos os vértices únicos.
    std::vector<glm::vec3> denseSupportVertices;
    std::vector<unsigned int> denseSupportIndices;
    constexpr int supportGridResolution = 100;
    denseSupportVertices.reserve(
        static_cast<std::size_t>(supportGridResolution + 1) *
        static_cast<std::size_t>(supportGridResolution + 1));
    for (int z = 0; z <= supportGridResolution; ++z)
    {
        for (int x = 0; x <= supportGridResolution; ++x)
        {
            denseSupportVertices.emplace_back(
                static_cast<float>(x), 0.0f, static_cast<float>(z));
        }
    }
    for (int z = 0; z < supportGridResolution; ++z)
    {
        for (int x = 0; x < supportGridResolution; ++x)
        {
            const unsigned int a = static_cast<unsigned int>(
                z * (supportGridResolution + 1) + x);
            const unsigned int b = a + 1;
            const unsigned int c = a + supportGridResolution + 1;
            const unsigned int d = c + 1;
            denseSupportIndices.insert(
                denseSupportIndices.end(), {a, c, b, b, c, d});
        }
    }
    rotor::physics::TriangleMeshColliderData denseSupportMesh;
    passed &= expect(
        rotor::physics::buildTriangleMeshColliderData(
            denseSupportVertices, denseSupportIndices, denseSupportMesh),
        "dense support mesh builds with a BVH");
    Transform denseSupportTransform;
    rotor::physics::RigidBody denseSupportBody;
    denseSupportBody.transform = &denseSupportTransform;
    denseSupportBody.collider.shape =
        rotor::physics::ColliderShape::TriangleMesh;
    denseSupportBody.collider.triangleMesh =
        std::make_shared<rotor::physics::TriangleMeshColliderData>(
            std::move(denseSupportMesh));
    rotor::physics::collision::MeshCollisionScratch denseSupportScratch;
    denseSupportScratch.collectStats = true;
    const glm::vec3 diagonalSupport = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
    const glm::vec3 denseSupportPoint =
        rotor::physics::collision::supportPoint(
            denseSupportBody, diagonalSupport, &denseSupportScratch);
    passed &= expect(
        denseSupportPoint.x > 49.9f && denseSupportPoint.z > 49.9f &&
            denseSupportScratch.counters.supportBvhNodesVisited <
                denseSupportBody.collider.triangleMesh->bvhNodes.size() / 2 &&
            denseSupportScratch.counters.supportVerticesTested <
                denseSupportBody.collider.triangleMesh->vertices.size() / 8,
        "dense mesh support query prunes most BVH nodes and vertices");

    // Uma malha com triângulos espacialmente separados deve consultar somente
    // os poucos nós próximos da caixa, em vez de percorrer toda a geometria.
    std::vector<glm::vec3> spatialVertices;
    std::vector<unsigned int> spatialIndices;
    for (int z = -5; z <= 5; ++z)
    for (int x = -5; x <= 5; ++x)
    {
        const unsigned int base = static_cast<unsigned int>(spatialVertices.size());
        const float px = static_cast<float>(x) * 2.0f;
        const float pz = static_cast<float>(z) * 2.0f;
        spatialVertices.emplace_back(px - 0.3f, 0.0f, pz - 0.3f);
        spatialVertices.emplace_back(px + 0.3f, 0.0f, pz - 0.3f);
        spatialVertices.emplace_back(px, 0.0f, pz + 0.3f);
        spatialIndices.insert(spatialIndices.end(), {base, base + 1, base + 2});
    }
    auto spatialMeshData =
        std::make_shared<rotor::physics::TriangleMeshColliderData>();
    passed &= expect(
        rotor::physics::buildTriangleMeshColliderData(
            spatialVertices, spatialIndices, *spatialMeshData),
        "spatially distributed triangle mesh builds its BVH");
    Transform spatialMeshTransform;
    rotor::physics::RigidBody spatialMeshBody;
    spatialMeshBody.transform = &spatialMeshTransform;
    spatialMeshBody.collider.shape = rotor::physics::ColliderShape::TriangleMesh;
    spatialMeshBody.collider.triangleMesh = spatialMeshData;
    spatialMeshBody.inverseMass = 1.0f;
    spatialMeshBody.synchronizeColliderAndInertia();
    const auto smallSpatialBox = rotor::physics::collision::makeOrientedBox(
        glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3(0.5f, 0.1f, 0.5f));
    rotor::physics::collision::MeshCollisionScratch spatialScratch;
    rotor::physics::ContactManifold spatialManifold;
    passed &= expect(
        rotor::physics::collision::triangleMeshOrientedBoxContacts(
            spatialMeshBody, smallSpatialBox, spatialManifold, &spatialScratch) &&
            spatialScratch.candidateTriangles.size() <
                spatialMeshData->triangleIndices.size() / 12,
        "BVH narrow phase visits only a small fraction of a large mesh");

    PhysicsWorld triangleMeshGroundWorld;
    triangleMeshGroundWorld.setGroundPlane(0.0f, 0.8f, 0.6f);
    Transform triangleMeshGroundBody;
    triangleMeshGroundBody.move(glm::vec3(0.0f, 2.0f, 0.0f));
    passed &= expect(
        triangleMeshGroundWorld.addTriangleMeshBody(
            triangleMeshGroundBody, meshBoxData, 0.0f, 1.0f),
        "dynamic triangle mesh body is registered");
    for (int i = 0; i < 480; ++i)
        triangleMeshGroundWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(triangleMeshGroundBody.position().y - 0.5f) < 0.001f &&
            glm::length(triangleMeshGroundWorld.angularVelocity(triangleMeshGroundBody)) < 1e-3f,
        "triangle mesh uses its actual bottom vertices when resting on a plane");

    PhysicsWorld triangleMeshTableWorld;
    Transform triangleMeshTable;
    triangleMeshTableWorld.addStaticBox(
        triangleMeshTable, glm::vec3(0.0f), glm::vec3(1.0f, 0.1f, 1.0f));
    Transform triangleMeshOnTable;
    triangleMeshOnTable.move(glm::vec3(0.0f, 1.5f, 0.0f));
    passed &= expect(
        triangleMeshTableWorld.addTriangleMeshBody(
            triangleMeshOnTable, meshBoxData, 0.0f, 1.0f),
        "triangle mesh body is registered above a static box collider");
    for (int i = 0; i < 480; ++i)
        triangleMeshTableWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(triangleMeshOnTable.position().y - 0.6f) < 0.0003f &&
            glm::length(triangleMeshTableWorld.angularVelocity(triangleMeshOnTable)) < 1e-3f,
        "triangle mesh uses its actual vertices on the static tabletop");

    // Uma malha profundamente penetrada no plano deve usar somente o
    // feature de suporte mais baixo, sem criar contatos nas laterais.
    Transform deepMeshTransform;
    deepMeshTransform.move(glm::vec3(0.0f, -0.3f, 0.0f));
    rotor::physics::RigidBody deepMeshBody;
    deepMeshBody.transform = &deepMeshTransform;
    deepMeshBody.collider.shape = rotor::physics::ColliderShape::TriangleMesh;
    deepMeshBody.collider.triangleMesh = meshBoxData;
    deepMeshBody.inverseMass = 1.0f;
    deepMeshBody.synchronizeColliderAndInertia();
    rotor::physics::ContactManifold deepMeshPlaneManifold;
    passed &= expect(
        rotor::physics::collision::triangleMeshPlaneContacts(
            deepMeshBody, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f,
            deepMeshPlaneManifold) && deepMeshPlaneManifold.pointCount == 4,
        "deep mesh-plane overlap uses the four-point lowest support feature");
    for (std::size_t i = 0; i < deepMeshPlaneManifold.pointCount; ++i)
    {
        passed &= expect(
            std::fabs(deepMeshPlaneManifold.points[i].pointB.y + 0.8f) < 1e-4f,
            "deep mesh-plane contacts do not use side vertices");
    }

    // A face can cross a collider even when all three original triangle
    // vertices are outside its bounds. The narrow phase must clip triangles,
    // not rely only on testing source vertices.
    const std::vector<glm::vec3> crossingTriangleVertices = {
        {-2.0f, 0.0f, -2.0f}, {2.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 2.0f}};
    const std::vector<unsigned int> crossingTriangleIndices = {0, 1, 2};
    auto crossingTriangleData =
        std::make_shared<rotor::physics::TriangleMeshColliderData>();
    passed &= expect(
        rotor::physics::buildTriangleMeshColliderData(
            crossingTriangleVertices, crossingTriangleIndices, *crossingTriangleData),
        "large crossing triangle mesh collider builds");
    Transform crossingTriangleTransform;
    rotor::physics::RigidBody crossingTriangleBody;
    crossingTriangleBody.transform = &crossingTriangleTransform;
    crossingTriangleBody.collider.shape = rotor::physics::ColliderShape::TriangleMesh;
    crossingTriangleBody.collider.triangleMesh = crossingTriangleData;
    crossingTriangleBody.inverseMass = 1.0f;
    crossingTriangleBody.synchronizeColliderAndInertia();
    const auto crossingTargetBox = rotor::physics::collision::makeOrientedBox(
        glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.1f, 1.0f));
    rotor::physics::ContactManifold crossingTriangleManifold;
    passed &= expect(
        rotor::physics::collision::triangleMeshOrientedBoxContacts(
            crossingTriangleBody, crossingTargetBox, crossingTriangleManifold) &&
            crossingTriangleManifold.pointCount > 0,
        "triangle-box clipping detects intersection even when no source vertex is inside");

    // Uma malha assimétrica deve deslocar o centro de massa para a região com
    // maior área superficial, em vez de usar automaticamente o centro do AABB.
    std::vector<glm::vec3> asymmetricVertices;
    std::vector<unsigned int> asymmetricIndices;
    const auto appendTestBox = [&](const glm::vec3& center, const glm::vec3& half)
    {
        const unsigned int base = static_cast<unsigned int>(asymmetricVertices.size());
        const glm::vec3 corners[8] = {
            center + glm::vec3(-half.x, -half.y, -half.z),
            center + glm::vec3( half.x, -half.y, -half.z),
            center + glm::vec3( half.x,  half.y, -half.z),
            center + glm::vec3(-half.x,  half.y, -half.z),
            center + glm::vec3(-half.x, -half.y,  half.z),
            center + glm::vec3( half.x, -half.y,  half.z),
            center + glm::vec3( half.x,  half.y,  half.z),
            center + glm::vec3(-half.x,  half.y,  half.z)};
        asymmetricVertices.insert(
            asymmetricVertices.end(), std::begin(corners), std::end(corners));
        const unsigned int localIndices[] = {
            0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
            0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5,
            2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
        for (unsigned int index : localIndices)
            asymmetricIndices.push_back(base + index);
    };
    appendTestBox(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.1f, 0.5f));
    appendTestBox(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.05f, 0.5f, 0.05f));
    rotor::physics::TriangleMeshColliderData asymmetricMeshData;
    passed &= expect(
        rotor::physics::buildTriangleMeshColliderData(
            asymmetricVertices, asymmetricIndices, asymmetricMeshData) &&
            asymmetricMeshData.localCenterOfMass.y < 0.2f,
        "surface-area center of mass shifts toward the larger base of an asymmetric mesh");

    // Lâmpada simplificada: base larga/pesada por área, haste estreita e cúpula.
    // Inclinada além do polígono de apoio, deve tombar em vez de ficar presa
    // numa face de uma OBB que não representa sua geometria.
    std::vector<glm::vec3> lampVertices;
    std::vector<unsigned int> lampIndices;
    const auto appendLampBox = [&](const glm::vec3& center, const glm::vec3& half)
    {
        const unsigned int base = static_cast<unsigned int>(lampVertices.size());
        const glm::vec3 corners[8] = {
            center + glm::vec3(-half.x, -half.y, -half.z),
            center + glm::vec3( half.x, -half.y, -half.z),
            center + glm::vec3( half.x,  half.y, -half.z),
            center + glm::vec3(-half.x,  half.y, -half.z),
            center + glm::vec3(-half.x, -half.y,  half.z),
            center + glm::vec3( half.x, -half.y,  half.z),
            center + glm::vec3( half.x,  half.y,  half.z),
            center + glm::vec3(-half.x,  half.y,  half.z)};
        lampVertices.insert(lampVertices.end(), std::begin(corners), std::end(corners));
        const unsigned int localIndices[] = {
            0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
            0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5,
            2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
        for (unsigned int index : localIndices)
            lampIndices.push_back(base + index);
    };
    appendLampBox(glm::vec3(0.0f, -0.55f, 0.0f), glm::vec3(0.4f, 0.08f, 0.4f));
    appendLampBox(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.025f, 0.55f, 0.025f));
    appendLampBox(glm::vec3(0.0f, 0.68f, 0.0f), glm::vec3(0.25f, 0.10f, 0.25f));

    auto lampMeshData = std::make_shared<rotor::physics::TriangleMeshColliderData>();
    passed &= expect(
        rotor::physics::buildTriangleMeshColliderData(
            lampVertices, lampIndices, *lampMeshData),
        "compound lamp mesh builds as one rigid collider");
    PhysicsWorld tiltedLampWorld;
    Transform tiltedLampTable;
    tiltedLampWorld.addStaticBox(
        tiltedLampTable, glm::vec3(0.0f), glm::vec3(2.0f, 0.1f, 2.0f),
        0.0f, 0.9f, 0.7f);
    Transform tiltedLamp;
    const glm::quat initialLampOrientation = glm::angleAxis(
        glm::radians(65.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    tiltedLamp.setOrientation(initialLampOrientation);
    float lampLowestY = std::numeric_limits<float>::max();
    for (const glm::vec3& vertex : lampMeshData->vertices)
        lampLowestY = std::min(
            lampLowestY, (initialLampOrientation * vertex).y);
    tiltedLamp.move(glm::vec3(0.0f, 0.1f - lampLowestY, 0.0f));
    passed &= expect(
        tiltedLampWorld.addTriangleMeshBody(
            tiltedLamp, lampMeshData, 0.0f, 2.0f, 0.9f, 0.7f),
        "tilted lamp mesh registers as one rigid body");
    for (int i = 0; i < 240; ++i)
        tiltedLampWorld.update(1.0f / 120.0f);
    const float lampRotationChange = 2.0f * std::acos(std::clamp(
        std::fabs(glm::dot(initialLampOrientation, tiltedLamp.orientation())),
        0.0f, 1.0f));
    passed &= expect(
        lampRotationChange > glm::radians(8.0f),
        "tilted compound lamp tips when its center of mass leaves the base support area");

    return passed ? 0 : 1;
}
