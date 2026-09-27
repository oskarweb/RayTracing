#pragma once

#include "core/extras.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Physics
{
struct Body
{
    Types::Vec3d position, velocity;
    double charge, mass;
    bool movable;
    Types::OdeMethod method;
};

inline Types::Vec3d coulombForce(Types::Vec3d displacement, double chargeProduct, double softening)
{
    const double radiusSquared = displacement.length2(softening);
    const double radius = displacement.length();
    if (radius == 0.0)
        return Types::Vec3d(0.0);
    return 8.9875517873681764e9 * chargeProduct * displacement / (radiusSquared * radius);
}

inline std::vector<Types::Vec3d> forces(const std::vector<Body> &bodies, double softening)
{
    std::vector<Types::Vec3d> result(bodies.size(), Types::Vec3d(0.0));
    for (size_t i = 0; i < bodies.size(); ++i)
        for (size_t j = i + 1; j < bodies.size(); ++j)
        {
            if (bodies[i].method != bodies[j].method)
                continue;
            const auto force =
                coulombForce(bodies[i].position - bodies[j].position, bodies[i].charge * bodies[j].charge, softening);
            result[i] += force;
            result[j] -= force;
        }
    return result;
}

// Each method is an independent comparison system. Evaluate every body at the
// same intermediate time, including fixed sources at their original position.
inline void advance(std::vector<Body> &bodies, double dt, double softening)
{
    if (!std::isfinite(dt) || dt <= 0.0 || !std::isfinite(softening) || softening < 0.0)
        throw std::invalid_argument("Invalid integration timestep or softening");
    for (const auto &body : bodies)
    {
        if (!std::isfinite(body.mass) || body.mass <= 0.0)
            throw std::invalid_argument("Particle mass must be positive and finite");
        if (body.method != Types::OdeMethod::RK4 && body.method != Types::OdeMethod::ForwardEuler &&
            body.method != Types::OdeMethod::Leapfrog)
            throw std::invalid_argument("Unknown integration method");
    }
    const auto initial = bodies;
    const auto f1 = forces(initial, softening);
    auto stage = initial;
    for (size_t i = 0; i < bodies.size(); ++i)
        if (initial[i].movable)
        {
            stage[i].position += initial[i].velocity * (dt / 2.0);
            stage[i].velocity += f1[i] * (dt / (2.0 * initial[i].mass));
        }
    const auto stage2 = stage;
    const auto f2 = forces(stage2, softening);
    for (size_t i = 0; i < bodies.size(); ++i)
        if (initial[i].movable)
        {
            stage[i].position = initial[i].position + stage2[i].velocity * (dt / 2.0);
            stage[i].velocity = initial[i].velocity + f2[i] * (dt / (2.0 * initial[i].mass));
        }
    const auto stage3 = stage;
    const auto f3 = forces(stage3, softening);
    for (size_t i = 0; i < bodies.size(); ++i)
        if (initial[i].movable)
        {
            stage[i].position = initial[i].position + stage3[i].velocity * dt;
            stage[i].velocity = initial[i].velocity + f3[i] * (dt / initial[i].mass);
        }
    const auto f4 = forces(stage, softening);
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        auto &body = bodies[i];
        if (!body.movable)
            continue;
        switch (body.method)
        {
        case Types::OdeMethod::RK4:
            body.position +=
                (initial[i].velocity + 2.0 * stage2[i].velocity + 2.0 * stage3[i].velocity + stage[i].velocity) *
                (dt / 6.0);
            body.velocity += (f1[i] + 2.0 * f2[i] + 2.0 * f3[i] + f4[i]) * (dt / (6.0 * body.mass));
            break;
        case Types::OdeMethod::ForwardEuler:
            body.position += body.velocity * dt;
            body.velocity += f1[i] * (dt / body.mass);
            break;
        case Types::OdeMethod::Leapfrog:
            body.position += body.velocity * dt + f1[i] * (dt * dt / (2.0 * body.mass));
            break;
        }
    }
    const auto finalForces = forces(bodies, softening);
    for (size_t i = 0; i < bodies.size(); ++i)
        if (bodies[i].movable && bodies[i].method == Types::OdeMethod::Leapfrog)
            bodies[i].velocity += (f1[i] + finalForces[i]) * (dt / (2.0 * bodies[i].mass));
}
} // namespace Physics
