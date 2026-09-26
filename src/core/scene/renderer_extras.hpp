#pragma once

#include <limits>
#include <stdexcept>

#include "core/resources/mesh.hpp"
#include "core/sparse_set.hpp"

#include <functional>

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

struct Material
{
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::Pipeline pipeline{nullptr};

    Material() = default;
    Material(vk::raii::Pipeline pipeline, vk::raii::PipelineLayout layout)
        : pipelineLayout(std::move(layout)), pipeline(std::move(pipeline))
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

struct Texture
{
    Image image{};
    vk::raii::ImageView imageView{nullptr};
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
    if (nx < 2 || nz < 2 || !formula)
        throw std::invalid_argument("Paraboloid requires dimensions >= 2 and a formula");
    const size_t columns = static_cast<size_t>(nx - 1);
    const size_t rows = static_cast<size_t>(nz - 1);
    if (columns > std::vector<Vertex>().max_size() / 6 / rows)
        throw std::length_error("Paraboloid dimensions are too large");
    const size_t verts = columns * rows * 6;

    std::vector<Vertex> mesh(verts);

    double dx = (xmax - xmin) / double(nx - 1);
    double dz = (zmax - zmin) / double(nz - 1);

    size_t idx = 0;

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