#include "core/camera/camera.hpp"
#include "core/camera/input.hpp"
#include "core/resources/mesh.hpp"
#include "core/scene/renderer_extras.hpp"
#include "test_support.hpp"

namespace
{
TEST(MeshTest, DeduplicatesCompleteVerticesAndReplacesPreviousGeometry)
{
    const Vertex first{{0, 0, 0}, {1, 0, 0}, {0, 0}};
    const Vertex second{{1, 0, 0}, {1, 0, 0}, {0, 0}};
    const Vertex seam{{0, 0, 0}, {1, 0, 0}, {1, 0}};
    const std::array vertices{first, second, first, seam};
    Mesh mesh;
    mesh.fromVertices(vertices);
    EXPECT_EQ(mesh.m_vertices.size(), 3u);
    EXPECT_EQ(mesh.m_indices, (std::vector<uint32_t>{0, 1, 0, 2}));
    mesh.fromVertices(std::span(&second, 1));
    EXPECT_EQ(mesh.m_vertices.size(), 1u);
    EXPECT_EQ(mesh.m_vertices.front(), second);
    EXPECT_EQ(mesh.m_indices, (std::vector<uint32_t>{0}));
}

TEST(MeshTest, EmptyAndUninitializedUploadsFailBeforeCallingVulkan)
{
    Mesh mesh;
    EXPECT_THROW(mesh.upload(nullptr, VK_NULL_HANDLE, VK_NULL_HANDLE), std::invalid_argument);
    const Vertex vertex{};
    mesh.fromVertices(std::span(&vertex, 1));
    EXPECT_THROW(mesh.upload(nullptr, VK_NULL_HANDLE, VK_NULL_HANDLE), std::logic_error);
    AllocatedBuffer buffer;
    EXPECT_THROW(buffer.write({}), std::logic_error);
}

TEST(CameraTest, OriginAndVerticalViewsRemainFiniteAndInvertible)
{
    for (const auto position : {glm::vec3(0), glm::vec3(0, 5, 0), glm::vec3(0, -5, 0)})
    {
        Camera camera;
        camera.position() = position;
        const auto view = camera.getViewMatrix();
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                EXPECT_TRUE(std::isfinite(view[column][row]));
        EXPECT_NEAR(std::abs(glm::determinant(view)), 1.0f, 1e-5f);
        expectVectorNear(glm::vec3(view * glm::vec4(position, 1)), glm::vec3(0));
    }
}

TEST(CameraTest, CalculatedAnglesPointLocalForwardAtTarget)
{
    for (const auto position : {glm::vec3(3, 4, 5), glm::vec3(-3, -4, -5)})
    {
        Camera camera;
        camera.position() = position;
        camera.calculatePitchYaw();
        expectVectorNear(glm::vec3(camera.getRotationMatrix() * glm::vec4(0, 0, 1, 0)), glm::normalize(-position));
    }
    Camera origin;
    origin.calculatePitchYaw();
    EXPECT_FLOAT_EQ(glm::determinant(origin.getRotationMatrix()), 1.0f);
}

class InputTest : public testing::Test
{
    void SetUp() override
    {
        for (auto &[key, state] : Input::keyStates)
            state = {};
        for (auto &[button, state] : Input::mouseButtonStates)
            state = {};
        Input::mousePos = {};
    }
    void TearDown() override { SetUp(); }
};

TEST_F(InputTest, UnknownInputsDoNotInsertStateOrThrow)
{
    const auto keys = Input::keyStates.size(), buttons = Input::mouseButtonStates.size();
    EXPECT_FALSE(Input::isPressedKeyboard(GLFW_KEY_UNKNOWN));
    EXPECT_FALSE(Input::isJustPressedKeyboard(GLFW_KEY_UNKNOWN));
    EXPECT_FALSE(Input::isPressedMouse(-1));
    EXPECT_FALSE(Input::isJustPressedMouse(-1));
    Input::keyCallback(nullptr, GLFW_KEY_UNKNOWN, 0, GLFW_PRESS, 0);
    Input::mouseButtonCallback(nullptr, -1, GLFW_PRESS, 0);
    EXPECT_EQ(Input::keyStates.size(), keys);
    EXPECT_EQ(Input::mouseButtonStates.size(), buttons);
}

TEST_F(InputTest, KeyboardPressIsConsumedOnceAndRepeatDoesNotRetrigger)
{
    Input::keyCallback(nullptr, GLFW_KEY_W, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(Input::isPressedKeyboard(GLFW_KEY_W));
    EXPECT_TRUE(Input::isJustPressedKeyboard(GLFW_KEY_W));
    EXPECT_FALSE(Input::isJustPressedKeyboard(GLFW_KEY_W));
    Input::keyCallback(nullptr, GLFW_KEY_W, 0, GLFW_REPEAT, 0);
    EXPECT_TRUE(Input::isPressedKeyboard(GLFW_KEY_W));
    EXPECT_FALSE(Input::isJustPressedKeyboard(GLFW_KEY_W));
    Input::keyCallback(nullptr, GLFW_KEY_W, 0, GLFW_RELEASE, 0);
    EXPECT_FALSE(Input::isPressedKeyboard(GLFW_KEY_W));
    EXPECT_FALSE(Input::isJustPressedKeyboard(GLFW_KEY_W));
    Input::keyCallback(nullptr, GLFW_KEY_W, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(Input::isJustPressedKeyboard(GLFW_KEY_W));
}

TEST_F(InputTest, QuickMouseClickRetainsPressUntilConsumed)
{
    Input::mouseButtonCallback(nullptr, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
    Input::mouseButtonCallback(nullptr, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
    EXPECT_FALSE(Input::isPressedMouse(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_TRUE(Input::isJustPressedMouse(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_FALSE(Input::isJustPressedMouse(GLFW_MOUSE_BUTTON_LEFT));
}

TEST_F(InputTest, CursorCallbackPreservesWindowCoordinates)
{
    Input::mouseCallback(nullptr, 123.25, -4.5);
    EXPECT_DOUBLE_EQ(Input::getMousePos().x, 123.25);
    EXPECT_DOUBLE_EQ(Input::getMousePos().y, -4.5);
}

TEST(ParaboloidMeshTest, RectangularGridCoversDomainAndEvaluatesSurface)
{
    const auto vertices =
        generateParaboloidVertices(3, 4, -2, 2, -3, 3, {}, [](double x, double z) { return x * x + z * z; });
    ASSERT_EQ(vertices.size(), 36u);
    glm::vec3 minimum(std::numeric_limits<float>::max()), maximum(std::numeric_limits<float>::lowest());
    for (const auto &vertex : vertices)
    {
        minimum = glm::min(minimum, vertex.pos);
        maximum = glm::max(maximum, vertex.pos);
        EXPECT_FLOAT_EQ(vertex.pos.y, -(vertex.pos.x * vertex.pos.x + vertex.pos.z * vertex.pos.z));
    }
    EXPECT_FLOAT_EQ(minimum.x, -2);
    EXPECT_FLOAT_EQ(maximum.x, 2);
    EXPECT_FLOAT_EQ(minimum.z, -3);
    EXPECT_FLOAT_EQ(maximum.z, 3);
    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        const auto normal = glm::cross(vertices[i + 1].pos - vertices[i].pos, vertices[i + 2].pos - vertices[i].pos);
        EXPECT_GT(glm::length(normal), 0.0f);
        EXPECT_LT(normal.y, 0.0f);
    }
}

TEST(ParaboloidMeshTest, RejectsDegenerateDimensionsAndMissingFormula)
{
    const auto formula = [](double, double) { return 0.0; };
    for (int dimension : {-2, 0, 1})
    {
        EXPECT_THROW(generateParaboloidVertices(dimension, 2, -1, 1, -1, 1, {}, formula), std::invalid_argument);
        EXPECT_THROW(generateParaboloidVertices(2, dimension, -1, 1, -1, 1, {}, formula), std::invalid_argument);
    }
    EXPECT_THROW(generateParaboloidVertices(2, 2, -1, 1, -1, 1, {}, {}), std::invalid_argument);
}
} // namespace
