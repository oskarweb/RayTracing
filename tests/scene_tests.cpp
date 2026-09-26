#include "core/vulkan/commands.hpp"
#include "raster/raster_renderer.hpp"
#include "raytracing/surface.hpp"
#include "simulation/particle.hpp"
#include "test_support.hpp"

TEST(RendererContractTest, UninitializedOperationsFailWithoutPublishingResources)
{
    VulkanRenderer renderer;
    EXPECT_THROW(renderer.init(), std::logic_error);
    EXPECT_THROW(renderer.newFrame(), std::logic_error);
    EXPECT_THROW(renderer.addRenderObject({}), std::invalid_argument);
    EXPECT_THROW(renderer.createParaboloid("failed", 2, 2, [](double, double) { return 0.0; }), std::logic_error);
    EXPECT_THROW(renderer.getMesh("failed"), std::out_of_range);
    EXPECT_NO_THROW(renderer.cleanup());
}

TEST(ShaderModuleTest, RejectsEmptyAndTruncatedSpirvBeforeCallingVulkan)
{
    const vk::raii::Device device(nullptr);
    EXPECT_THROW(Helpers::createShaderModule(device, {}), std::invalid_argument);
    const std::array truncated{'a', 'b', 'c'};
    EXPECT_THROW(Helpers::createShaderModule(device, truncated), std::invalid_argument);
}
TEST_F(SceneTest, SurfaceUsesRequestedMeshAndCleansUpIdempotently)
{
    Surface surface("custom-surface", Types::Vec3d(1.0));
    EXPECT_EQ(renderer.requestedMesh, "custom-surface");
    ASSERT_EQ(renderer.objects.dense().size(), 1u);
    EXPECT_THROW(surface.getModel("missing"), std::out_of_range);
    surface.cleanup();
    EXPECT_NO_THROW(surface.cleanup());
    EXPECT_TRUE(renderer.objects.dense().empty());
}

TEST_F(SceneTest, ReplacingNamedModelReleasesPreviousRenderObject)
{
    Surface surface("custom", Types::Vec3d(0.0));
    surface.uploadModel("surface", std::make_unique<ParticleModel>(glm::vec3(2.0f)));
    EXPECT_EQ(renderer.objects.dense().size(), 1u);
    EXPECT_NE(dynamic_cast<ParticleModel *>(surface.getModel("surface")), nullptr);
    EXPECT_THROW(surface.uploadModel("invalid", nullptr), std::invalid_argument);
    EXPECT_THROW(surface.getModel("invalid"), std::out_of_range);
}

TEST_F(SceneTest, ModelDestructionReleasesRenderObjects)
{
    {
        AxesModel axes(glm::vec3(0.0f));
        EXPECT_EQ(renderer.objects.dense().size(), 3u);
    }
    EXPECT_TRUE(renderer.objects.dense().empty());
}

TEST_F(SceneTest, ModelKeepsOriginalRendererWhenGlobalRendererChanges)
{
    auto model = std::make_unique<ParticleModel>(glm::vec3(0.0f));
    TestRenderer other;
    BaseObject::setRenderer(&other);
    model->update(glm::vec3(3.0f));
    ASSERT_EQ(renderer.objects.dense().size(), 1u);
    EXPECT_TRUE(other.objects.dense().empty());
    expectVectorNear(glm::vec3(renderer.objects.dense().front().value.transformMatrix[3]), glm::vec3(3.0f));
    model.reset();
    EXPECT_TRUE(renderer.objects.dense().empty());
}

TEST_F(SceneTest, CleanedRendererRejectsUpdatesAndAllowsModelDestruction)
{
    auto model = std::make_unique<ParticleModel>(glm::vec3(0.0f));
    renderer.cleanup();
    EXPECT_THROW(model->update(), std::logic_error);
    EXPECT_NO_THROW(model.reset());
    EXPECT_THROW(ParticleModel(glm::vec3(0.0f)), std::logic_error);
}

TEST_F(SceneTest, OldModelCannotRemoveReinitializedRenderersObjects)
{
    auto old = std::make_unique<ParticleModel>(glm::vec3(0.0f));
    renderer.cleanup();
    renderer.reinitialize();
    ParticleModel current(glm::vec3(1.0f));
    old.reset();
    EXPECT_EQ(renderer.objects.dense().size(), 1u);
    EXPECT_NO_THROW(current.update());
}

TEST_F(SceneTest, ModelCanOutliveRenderer)
{
    auto temporary = std::make_unique<TestRenderer>();
    BaseObject::setRenderer(temporary.get());
    auto model = std::make_unique<ParticleModel>(glm::vec3(0.0f));
    temporary.reset();
    EXPECT_THROW(model->update(), std::logic_error);
    EXPECT_NO_THROW(model.reset());
    EXPECT_THROW(ParticleModel(glm::vec3(0.0f)), std::logic_error);
}

TEST_F(SceneTest, LineTransformMapsUnitEndpointsToRequestedPoints)
{
    const glm::vec3 from(1, -2, 3), to(-4, 5, 6);
    RedLineModel line(from, to);
    const auto matrix = line.renderObject("trail")->transformMatrix;
    expectVectorNear(glm::vec3(matrix * glm::vec4(0, 0, 0, 1)), from);
    expectVectorNear(glm::vec3(matrix * glm::vec4(1, 0, 0, 1)), to);
    line.update(from, to - from, glm::vec3(glm::length(to - from), 1, 1));
    const auto updated = line.renderObject("trail")->transformMatrix;
    expectVectorNear(glm::vec3(updated * glm::vec4(1, 0, 0, 1)), to);
}

TEST_F(SceneTest, ZeroLengthLinesAndZeroForceArrowHaveFiniteTransforms)
{
    RedLineModel red(glm::vec3(2.0f), glm::vec3(2.0f));
    YellowLineModel yellow(glm::vec3(0.0f), glm::vec3(0.0f));
    GreenLineModel green(glm::vec3(0.0f), glm::vec3(0.0f));
    VectorArrowModel arrow(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    arrow.update();
    for (const auto &entry : renderer.objects.dense())
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                EXPECT_TRUE(std::isfinite(entry.value.transformMatrix[column][row]));
}

TEST_F(SceneTest, UpdatingCuboidAndSurfacePreservesScale)
{
    const glm::vec3 scale(2, 3, 4), position(5, 6, 7);
    CuboidModel cuboid(glm::vec3(0.0f), scale);
    ParaboloidModel surface(glm::vec3(0.0f), scale, "custom");
    cuboid.update(position);
    surface.update(position);
    for (const auto &entry : renderer.objects.dense())
    {
        const auto &matrix = entry.value.transformMatrix;
        expectVectorNear(glm::vec3(matrix * glm::vec4(0, 0, 0, 1)), position);
        expectVectorNear(glm::vec3(matrix * glm::vec4(1, 1, 1, 1)), position + scale);
    }
}

TEST_F(SceneTest, ParticleAccelerationUsesElapsedTime)
{
    Particle particle(0.0, 2.0, true, Types::Vec3d(0.0), Types::Vec3d(0.0), Types::OdeMethod::RK4);
    particle.update(Types::Vec3d(4, 0, 0), 0.25);
    expectVectorNear(particle.getVelocity(), Types::Vec3d(0.5, 0, 0));
    expectVectorNear(particle.getPos(), Types::Vec3d(0.125, 0, 0));
    particle.cleanup();
    EXPECT_NO_THROW(particle.cleanup());
    EXPECT_EQ(particle.getMaxStep(), 0u);
}

TEST_F(SceneTest, MissingPrecalculatedStateDoesNotChangeParticle)
{
    Particle particle(0.0, 1.0, true, Types::Vec3d(2.0), Types::Vec3d(0.0), Types::OdeMethod::RK4);
    EXPECT_FALSE(particle.updateFromPrecalcPos(99));
    expectVectorNear(particle.getPos(), Types::Vec3d(2.0));
    EXPECT_TRUE(particle.statesData().empty());
}
