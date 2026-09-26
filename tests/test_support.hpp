#pragma once

#include "core/scene/models.hpp"
#include <gtest/gtest.h>

// Records scene objects without creating a Vulkan instance, device, or window.
class TestRenderer : public Renderer
{
public:
    SparseSet<RenderObject> objects;
    std::string requestedMesh;
    SparseSet<RenderObject>::Handle addRenderObject(RenderObject obj) override { return objects.insert(obj); }
    void removeRenderObject(SparseSet<RenderObject>::Handle handle) override { objects.remove(handle); }
    SparseSet<Material>::Handle getMaterial(const std::string &) override { return {}; }
    SparseSet<Mesh>::Handle getMesh(const std::string &name) override
    {
        requestedMesh = name;
        return {};
    }
    RenderObject *getRenderObject(SparseSet<RenderObject>::Handle handle) override { return objects.get(handle); }
    void cleanup()
    {
        invalidateModels();
        objects = {};
    }
    void reinitialize() { renewModelLifetime(); }
};

class SceneTest : public testing::Test
{
protected:
    TestRenderer renderer;
    void SetUp() override { BaseObject::setRenderer(&renderer); }
    void TearDown() override { BaseObject::setRenderer(nullptr); }
};

inline void expectVectorNear(Types::Vec3d actual, Types::Vec3d expected, double tolerance = 1e-12)
{
    EXPECT_NEAR(actual.x(), expected.x(), tolerance);
    EXPECT_NEAR(actual.y(), expected.y(), tolerance);
    EXPECT_NEAR(actual.z(), expected.z(), tolerance);
}

inline void expectVectorNear(glm::vec3 actual, glm::vec3 expected, float tolerance = 1e-5f)
{
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
}
