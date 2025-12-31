#pragma once

#include "mesh.hpp"
#include "sparse_set.hpp"

#include <functional>

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

struct Material
{
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    Material() : pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {}
    Material(VkPipeline pipeline, VkPipelineLayout pipelineLayout) : pipeline(pipeline), pipelineLayout(pipelineLayout)
    {
    }
};

struct RenderObject
{
    RenderObject() = default;
    RenderObject(SparseSet<Mesh>::Handle mesh, SparseSet<Material>::Handle material, glm::mat4 tMat)
        : hMesh(mesh), hMaterial(material), transformMatrix(tMat)
    {
    }
    SparseSet<Mesh>::Handle hMesh{};
    SparseSet<Material>::Handle hMaterial{};
    glm::mat4 transformMatrix{};

    int textureIdx = -1;
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

struct Ray
{
    glm::vec3 origin{};
    glm::vec3 dir{};
};

inline const std::vector<Vertex> cubeVertices = {
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

inline const std::vector<Vertex> pyramidVertices = {
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

inline const std::vector<Vertex> redLineVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

inline const std::vector<Vertex> yellowLineVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline const std::vector<Vertex> greenLineVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f}, {0.0f, 0.0f}},
};

inline const std::vector<Vertex> xAxisVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{Constants::AXES_LENGTH, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

inline const std::vector<Vertex> yAxisVertices = {
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{0.0f, Constants::AXES_LENGTH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline const std::vector<Vertex> zAxisVertices = {
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