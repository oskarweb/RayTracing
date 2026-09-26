#include "simulation/integration.hpp"
#include "test_support.hpp"
#include <limits>

namespace
{
using Types::OdeMethod;
using Types::Vec3d;

TEST(CoulombForceTest, LikeChargesRepelAndUnlikeChargesAttract)
{
    const auto repulsion = Physics::coulombForce(Vec3d(2, 0, 0), 1e-12, 0.0);
    expectVectorNear(repulsion, Vec3d(0.002246887946842044, 0, 0));
    expectVectorNear(Physics::coulombForce(Vec3d(2, 0, 0), -1e-12, 0.0), -1.0 * repulsion);
}

TEST(CoulombForceTest, DoublingSeparationQuartersForce)
{
    const auto nearForce = Physics::coulombForce(Vec3d(1, 2, 2), 1e-12, 0.0);
    const auto farForce = Physics::coulombForce(Vec3d(2, 4, 4), 1e-12, 0.0);
    expectVectorNear(farForce, nearForce / 4.0);
}

TEST(CoulombForceTest, NeutralAndCoincidentParticlesHaveFiniteZeroForce)
{
    expectVectorNear(Physics::coulombForce(Vec3d(1, 2, 3), 0.0, 0.0), Vec3d(0.0));
    for (double softening : {0.0, 0.6})
        expectVectorNear(Physics::coulombForce(Vec3d(0.0), 1.0, softening), Vec3d(0.0));
    expectVectorNear(Vec3d(0.0).normalized(), Vec3d(0.0));
}

TEST(CoulombForceTest, SofteningReducesMagnitudeWithoutChangingDirection)
{
    const auto force = Physics::coulombForce(Vec3d(1, 0, 0), 1e-12, 0.0);
    const auto softened = Physics::coulombForce(Vec3d(1, 0, 0), 1e-12, 1.0);
    expectVectorNear(softened, force / 2.0);
}

TEST(CoulombForceTest, PairForcesSumToZeroForThreeBodies)
{
    std::vector<Physics::Body> bodies = {{Vec3d(-1, 0, 0), {}, 1e-5, 1, true, OdeMethod::RK4},
                                         {Vec3d(2, 1, 0), {}, -2e-5, 2, true, OdeMethod::RK4},
                                         {Vec3d(0, 3, 1), {}, 3e-5, 3, true, OdeMethod::RK4}};
    const auto forces = Physics::forces(bodies, 0.1);
    ASSERT_EQ(forces.size(), 3u);
    expectVectorNear(forces[0] + forces[1] + forces[2], Vec3d(0.0));
    EXPECT_GT(forces[0].length(), 0.0);
}

TEST(CoulombForceTest, DifferentMethodComparisonSystemsDoNotInteract)
{
    std::vector<Physics::Body> bodies = {{Vec3d(-1, 0, 0), {}, 1e-5, 1, true, OdeMethod::RK4},
                                         {Vec3d(1, 0, 0), {}, 1e-5, 1, true, OdeMethod::Leapfrog}};
    for (auto force : Physics::forces(bodies, 0.0))
        expectVectorNear(force, Vec3d(0.0));
}

class IntegratorTest : public testing::TestWithParam<OdeMethod>
{
};

TEST_P(IntegratorTest, FreeParticleFollowsAnalyticConstantVelocityMotion)
{
    const Vec3d position(1, -2, 3), velocity(2, 4, -6);
    std::vector<Physics::Body> bodies = {{position, velocity, 0, 2, true, GetParam()}};
    for (int step = 0; step < 100; ++step)
        Physics::advance(bodies, 0.01, 0.0);
    expectVectorNear(bodies[0].position, position + velocity);
    expectVectorNear(bodies[0].velocity, velocity);
}

TEST_P(IntegratorTest, ConservesTotalMomentumForUnequalMasses)
{
    std::vector<Physics::Body> bodies = {{Vec3d(-1, 0, 0), Vec3d(0, 0.2, 0), 1e-5, 1, true, GetParam()},
                                         {Vec3d(1, 0, 0), Vec3d(0, -0.1, 0), 1e-5, 2, true, GetParam()}};
    for (int step = 0; step < 100; ++step)
        Physics::advance(bodies, 0.01, 0.1);
    expectVectorNear(bodies[0].velocity + 2.0 * bodies[1].velocity, Vec3d(0.0));
}

TEST_P(IntegratorTest, FixedSourceStaysPutWhileAcceleratingMovingParticle)
{
    std::vector<Physics::Body> bodies = {{Vec3d(3, 0, 0), Vec3d(9, 0, 0), 1e-5, 1, false, GetParam()},
                                         {Vec3d(5, 0, 0), {}, -1e-5, 1, true, GetParam()}};
    for (int step = 0; step < 10; ++step)
        Physics::advance(bodies, 0.01, 0.1);
    expectVectorNear(bodies[0].position, Vec3d(3, 0, 0));
    expectVectorNear(bodies[0].velocity, Vec3d(9, 0, 0));
    EXPECT_LT(bodies[1].position.x(), 5.0);
    EXPECT_LT(bodies[1].velocity.x(), 0.0);
}

TEST_P(IntegratorTest, RejectsInvalidTimestepWithoutChangingState)
{
    for (double dt : {0.0, -0.01, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
    {
        SCOPED_TRACE(dt);
        std::vector<Physics::Body> bodies = {{Vec3d(1), Vec3d(2), 0, 1, true, GetParam()}};
        EXPECT_THROW(Physics::advance(bodies, dt, 0.1), std::invalid_argument);
        expectVectorNear(bodies[0].position, Vec3d(1));
        expectVectorNear(bodies[0].velocity, Vec3d(2));
    }
}

TEST_P(IntegratorTest, RejectsInvalidMass)
{
    for (double mass : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
    {
        SCOPED_TRACE(mass);
        std::vector<Physics::Body> bodies = {{Vec3d(0), {}, 0, mass, true, GetParam()}};
        EXPECT_THROW(Physics::advance(bodies, 0.01, 0.1), std::invalid_argument);
    }
}

INSTANTIATE_TEST_SUITE_P(AllMethods, IntegratorTest,
                         testing::Values(OdeMethod::RK4, OdeMethod::ForwardEuler, OdeMethod::Leapfrog),
                         [](const testing::TestParamInfo<OdeMethod> &info) {
                             switch (info.param)
                             {
                             case OdeMethod::RK4:
                                 return "RK4";
                             case OdeMethod::ForwardEuler:
                                 return "ForwardEuler";
                             default:
                                 return "Leapfrog";
                             }
                         });

TEST(IntegratorValidationTest, RejectsInvalidSofteningAndMethod)
{
    std::vector<Physics::Body> bodies = {{Vec3d(0), {}, 0, 1, true, OdeMethod::RK4}};
    for (double softening : {-1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
        EXPECT_THROW(Physics::advance(bodies, 0.01, softening), std::invalid_argument);
    bodies[0].method = static_cast<OdeMethod>(0);
    EXPECT_THROW(Physics::advance(bodies, 0.01, 0.1), std::invalid_argument);
}

TEST(Rk4Test, CircularTwoBodyOrbitConvergesAtFourthOrder)
{
    // Equal masses at unit radius have unit angular speed for this charge.
    const double charge = std::sqrt(4.0 / 8.9875517873681764e9);
    auto orbitError = [&](int steps) {
        std::vector<Physics::Body> bodies = {{Vec3d(-1, 0, 0), Vec3d(0, -1, 0), charge, 1, true, OdeMethod::RK4},
                                             {Vec3d(1, 0, 0), Vec3d(0, 1, 0), -charge, 1, true, OdeMethod::RK4}};
        for (int step = 0; step < steps; ++step)
            Physics::advance(bodies, 1.0 / steps, 0.0);
        return (bodies[1].position - Vec3d(std::cos(1.0), std::sin(1.0), 0)).length();
    };
    const double coarse = orbitError(10), fine = orbitError(20);
    EXPECT_GT(coarse, 0.0);
    EXPECT_LT(fine, coarse / 12.0);
    EXPECT_LT(fine, 1e-5);
}
} // namespace
