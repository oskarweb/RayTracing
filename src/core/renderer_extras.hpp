#pragma once

#include "mesh.hpp"

#include <functional>

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

struct Material
{
    std::string name = "default";
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    Material() : pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {}
    Material(const std::string &name, VkPipeline pipeline, VkPipelineLayout pipelineLayout)
        : name(name), pipeline(pipeline), pipelineLayout(pipelineLayout)
    {
    }
};

struct Renderable
{
    Mesh *mesh = nullptr;
    Material *material = nullptr;
    glm::mat4 transformMatrix{};

    std::unique_ptr<int> textureIdx = nullptr;
    uint64_t id = 0;
};

struct RenderableComp
{
    bool operator()(const std::string &lhs, const std::string &rhs) const { return lhs < rhs; }
};

struct Image
{
    VkImage data = VK_NULL_HANDLE;
    VmaAllocation allocation{};
};

struct Texture
{
    Image image{};
    VkImageView imageView = VK_NULL_HANDLE;
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
};

struct CameraBuffer
{
    glm::mat4 view{};
    glm::mat4 proj{};
};

struct RenderableInfo
{
    RenderableInfo(const std::string &renderableName, const std::string &meshName, const std::string &materialName,
                   glm::mat4 transformMatrix)
        : renderableName(renderableName), meshName(meshName), materialName(materialName),
          transformMatrix(transformMatrix)
    {
    }

    std::string renderableName{};
    std::string meshName{};
    std::string materialName{};
    glm::mat4 transformMatrix{};
};

inline constexpr const std::array<Vertex, 36> cubeVertices = {
    // Front
    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    // Back
    Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    // Left
    Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Right
    Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

    Vertex{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Top
    Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    // Bottom
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 18> pyramidVertices = {
    // Front
    Vertex{{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Back
    Vertex{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Left
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    // Right
    Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    // Bottom
    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> redLineVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> yellowLineVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> greenLineVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> xAxisVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{Constants::AXES_LENGTH, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> yAxisVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, Constants::AXES_LENGTH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> zAxisVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, 0.0f, Constants::AXES_LENGTH}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
};

inline std::vector<Vertex> generateParaboloidVertices(int nx, int nz, double xmin, double xmax, double zmin,
                                                      double zmax, glm::vec3 color,
                                                      const std::function<double(double, double)> &formula)
{
    int tris = (nx - 1) * (nz - 1) * 2;
    int verts = tris * 3;

    std::vector<Vertex> mesh(verts);

    double dx = (xmax - xmin) / double(nx - 1);
    double dz = (zmax - zmin) / double(nz - 1);

    int idx = 0;

    for (int i = 0; i < nx - 1; i++)
    {
        for (int j = 0; j < nz - 1; j++)
        {
            double x0 = xmin + i * dx;
            double x1 = xmin + (i + 1) * dx;
            double z0 = zmin + j * dz;
            double z1 = zmin + (j + 1) * dz;

            double y00 = formula(x0, z0);
            double y10 = formula(x1, z0);
            double y01 = formula(x0, z1);
            double y11 = formula(x1, z1);

            mesh[idx++] = Vertex{{x0, -y00, z0}, color, {0.0f, 0.0f}};
            mesh[idx++] = Vertex{{x1, -y10, z0}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
            mesh[idx++] = Vertex{{x0, -y01, z1}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}};

            mesh[idx++] = Vertex{{x1, -y10, z0}, color, {0.0f, 0.0f}};
            mesh[idx++] = Vertex{{x1, -y11, z1}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
            mesh[idx++] = Vertex{{x0, -y01, z1}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}};
        }
    }

    return mesh;
}