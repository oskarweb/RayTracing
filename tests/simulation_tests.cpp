#include "simulation/simulation.hpp"
#include "test_support.hpp"

// Keep access to the application state in its existing friend adapter; tests
// exercise normal simulation operations without opening the application's UI.
struct SimulationRegressionAccess
{
    static void configure(Simulation &s, double timestep, double duration = 0.1)
    {
        s.m_timeStep = timestep;
        s.m_simulationTime = duration;
    }
    static auto stepsPerBatch(Simulation &s) { return s.getStepsPer20ms(); }
    static auto &particles(Simulation &s) { return s.m_particles; }
    static void add(Simulation &s, Particle particle) { s.addParticle(std::move(particle)); }
    static void start(Simulation &s) { s.startSimulation(); }
    static void restart(Simulation &s) { s.restartSimulation(); }
    static void reset(Simulation &s) { s.resetAll(); }
    static void buffer(Simulation &s) { s.calculateParticlePositions(); }
    static void frame(Simulation &s) { s.update20MsPecalc(); }
    static void precalculateAll(Simulation &s) { s.m_mode = Simulation::SimulationMode::PrecalculatedAll; }
    static void removeFirst(Simulation &s)
    {
        auto it = s.m_particles.begin();
        s.removeParticle(it);
    }
    static auto maxStep(Simulation &s) { return s.m_mutualMaxStep; }
    static bool started(Simulation &s) { return s.m_hasStarted; }
    static bool paused(Simulation &s) { return s.m_paused; }
    static auto selected(Simulation &s) { return s.m_plotSelectedParticle; }
};

namespace
{
using Access = SimulationRegressionAccess;
using Types::Vec3d;

class SimulationTest : public SceneTest
{
protected:
    // Constructing this renderer does not initialize GLFW or Vulkan. Its default
    // delta time is zero, providing deterministic playback for buffer tests.
    VulkanRenderer clockRenderer;
    Simulation simulation{&clockRenderer};
    void SetUp() override
    {
        SceneTest::SetUp();
        Particle::distanceSoftening = Particle::DEFAULT_DISTANCE_SOFTENING;
        Access::configure(simulation, 0.01);
    }
    void addPair()
    {
        Access::add(simulation, Particle(1e-5, 1, false, Vec3d(3, 0, 0), Vec3d(0), Types::OdeMethod::RK4));
        Access::add(simulation, Particle(-1e-5, 1, true, Vec3d(5, 0, 0), Vec3d(0), Types::OdeMethod::RK4));
    }
};

TEST_F(SimulationTest, BatchHasAtLeastOneStepForLargeTimestep)
{
    Access::configure(simulation, 0.03);
    EXPECT_EQ(Access::stepsPerBatch(simulation), 1u);
    Access::configure(simulation, 0.0001);
    EXPECT_EQ(Access::stepsPerBatch(simulation), 200u);
}

TEST_F(SimulationTest, InvalidBatchTimestepIsRejected)
{
    for (double timestep :
         {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
    {
        Access::configure(simulation, timestep);
        EXPECT_THROW(Access::stepsPerBatch(simulation), std::invalid_argument);
    }
}

TEST_F(SimulationTest, EmptySimulationDoesNotStart)
{
    Access::start(simulation);
    EXPECT_TRUE(Access::paused(simulation));
    EXPECT_FALSE(Access::started(simulation));
}

TEST_F(SimulationTest, StartComputesInitialForceAndBufferRetainsFixedSource)
{
    addPair();
    Access::start(simulation);
    EXPECT_TRUE(Access::started(simulation));
    EXPECT_FALSE(Access::paused(simulation));
    auto &particles = Access::particles(simulation);
    ASSERT_EQ(particles.size(), 2u);
    ASSERT_TRUE(particles[1].statesData().contains(0));
    EXPECT_LT(particles[1].statesData().at(0).acceleration.x(), 0);
    Access::buffer(simulation);
    Access::buffer(simulation);
    EXPECT_EQ(Access::maxStep(simulation), 4u);
    ASSERT_TRUE(particles[0].statesData().contains(4));
    expectVectorNear(particles[0].statesData().at(4).pos, Vec3d(3, 0, 0));
    EXPECT_LT(particles[1].statesData().at(4).pos.x(), 5.0);
}

TEST_F(SimulationTest, StationaryPlaybackDoesNotGrowBuffer)
{
    addPair();
    Access::start(simulation);
    Access::buffer(simulation);
    const auto maxStep = Access::maxStep(simulation);
    const auto stateCount = Access::particles(simulation)[1].statesData().size();
    for (int frame = 0; frame < 100; ++frame)
        Access::frame(simulation);
    EXPECT_EQ(Access::maxStep(simulation), maxStep);
    EXPECT_EQ(Access::particles(simulation)[1].statesData().size(), stateCount);
}

TEST_F(SimulationTest, RepeatedPlaybackStepDoesNotDuplicateTrail)
{
    addPair();
    Access::start(simulation);
    Access::buffer(simulation);
    auto &particle = Access::particles(simulation)[1];
    ASSERT_TRUE(particle.updateFromPrecalcPos(1));
    const auto count = renderer.objects.dense().size();
    ASSERT_TRUE(particle.updateFromPrecalcPos(1));
    EXPECT_EQ(renderer.objects.dense().size(), count);
    ASSERT_TRUE(particle.updateFromPrecalcPos(2));
    EXPECT_EQ(renderer.objects.dense().size(), count + 1);
}

TEST_F(SimulationTest, RestartRestoresInitialPositionsAndClearsProgress)
{
    addPair();
    Access::start(simulation);
    Access::buffer(simulation);
    ASSERT_TRUE(Access::particles(simulation)[1].updateFromPrecalcPos(2));
    Access::restart(simulation);
    EXPECT_TRUE(Access::paused(simulation));
    EXPECT_FALSE(Access::started(simulation));
    EXPECT_EQ(Access::maxStep(simulation), 0u);
    expectVectorNear(Access::particles(simulation)[1].getPos(), Vec3d(5, 0, 0));
    EXPECT_TRUE(Access::particles(simulation)[1].statesData().empty());
    Access::start(simulation);
    Access::buffer(simulation);
    EXPECT_EQ(Access::maxStep(simulation), 2u);
}

TEST_F(SimulationTest, RemovedSourceDoesNotAffectRestartedSimulation)
{
    addPair();
    Access::start(simulation);
    Access::buffer(simulation);
    Access::removeFirst(simulation);
    Access::start(simulation);
    Access::buffer(simulation);
    ASSERT_EQ(Access::particles(simulation).size(), 1u);
    expectVectorNear(Access::particles(simulation).front().statesData().at(2).pos, Vec3d(5, 0, 0));
}

TEST_F(SimulationTest, ResetReleasesSceneAndClearsPlotSelection)
{
    addPair();
    Access::start(simulation);
    Access::buffer(simulation);
    Access::reset(simulation);
    EXPECT_TRUE(Access::particles(simulation).empty());
    EXPECT_FALSE(Access::selected(simulation).has_value());
    EXPECT_TRUE(renderer.objects.dense().empty());
    EXPECT_TRUE(Access::paused(simulation));
}

TEST_F(SimulationTest, FullAndBatchedPrecalculationProduceSameFinalState)
{
    addPair();
    Access::start(simulation);
    for (int batch = 0; batch < 5; ++batch)
        Access::buffer(simulation);
    ASSERT_EQ(Access::maxStep(simulation), 10u);
    const auto batched = Access::particles(simulation)[1].statesData().at(10);
    Access::precalculateAll(simulation);
    Access::start(simulation);
    ASSERT_EQ(Access::maxStep(simulation), 10u);
    const auto all = Access::particles(simulation)[1].statesData().at(10);
    expectVectorNear(all.pos, batched.pos);
    expectVectorNear(all.velocity, batched.velocity);
    expectVectorNear(all.acceleration, batched.acceleration);
    Access::buffer(simulation);
    EXPECT_EQ(Access::maxStep(simulation), 10u);
}
} // namespace
