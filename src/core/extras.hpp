#pragma once

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "imgui.h"

#include <immintrin.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <vector>

#define INTEL_AVX2

namespace Types
{
struct Vec3d
{
#ifdef INTEL_AVX2
    double v[4];

    Vec3d() : v(0.0, 0.0, 0.0, 0.0) {}
    Vec3d(double x, double y, double z) : v(x, y, z, 0.0) {}
    Vec3d(double n) : v(n, n, n, 0.0) {}

    double &x() { return v[0]; }
    double &y() { return v[1]; }
    double &z() { return v[2]; }

    double x() const { return v[0]; }
    double y() const { return v[1]; }
    double z() const { return v[2]; }

    inline Vec3d operator+(const Vec3d &o) const
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_loadu_pd(o.v);
        __m256d r = _mm256_add_pd(a, b);
        Vec3d out;
        _mm256_storeu_pd(out.v, r);
        return out;
    }

    inline Vec3d &operator+=(const Vec3d &o)
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_loadu_pd(o.v);
        __m256d r = _mm256_add_pd(a, b);
        _mm256_storeu_pd(v, r);
        return *this;
    }

    inline Vec3d operator-(const Vec3d &o) const
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_loadu_pd(o.v);
        __m256d r = _mm256_sub_pd(a, b);
        Vec3d out;
        _mm256_storeu_pd(out.v, r);
        return out;
    }

    inline Vec3d &operator-=(const Vec3d &o)
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_loadu_pd(o.v);
        __m256d r = _mm256_sub_pd(a, b);
        _mm256_storeu_pd(v, r);
        return *this;
    }

    inline Vec3d operator*(double s) const
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_set1_pd(s);
        __m256d r = _mm256_mul_pd(a, b);
        Vec3d out;
        _mm256_storeu_pd(out.v, r);
        return out;
    }

    inline Vec3d &operator*=(double s)
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_set1_pd(s);
        __m256d r = _mm256_mul_pd(a, b);
        _mm256_storeu_pd(v, r);
        return *this;
    }

    inline Vec3d operator/(double s) const
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_set1_pd(s);
        __m256d r = _mm256_div_pd(a, b);
        Vec3d out;
        _mm256_storeu_pd(out.v, r);
        return out;
    }

    inline Vec3d &operator/=(double s)
    {
        __m256d a = _mm256_loadu_pd(v);
        __m256d b = _mm256_set1_pd(s);
        __m256d r = _mm256_div_pd(a, b);
        _mm256_storeu_pd(v, r);
        return *this;
    }

    friend inline Vec3d operator*(double s, const Vec3d &v)
    {
        __m256d a = _mm256_loadu_pd(v.v);
        __m256d b = _mm256_set1_pd(s);
        __m256d r = _mm256_mul_pd(a, b);

        Vec3d out;
        _mm256_storeu_pd(out.v, r);
        return out;
    }

    friend inline Vec3d operator+(double s, const Vec3d &v)
    {
        __m256d va = _mm256_loadu_pd(v.v);
        __m256d vb = _mm256_set1_pd(s);
        __m256d r = _mm256_add_pd(va, vb);

        Vec3d out;
        _mm256_storeu_pd(out.v, r);
        return out;
    }
#else
    double m_x;
    double m_y;
    double m_z;

    double &x() { return m_x; }
    double &y() { return m_y; }
    double &z() { return m_z; }

    double x() const { return m_x; }
    double y() const { return m_y; }
    double z() const { return m_z; }
    constexpr Vec3d() : m_x(0.0), m_y(0.0), m_z(0.0) {}
    constexpr Vec3d(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}
    constexpr Vec3d(double n) : m_x(n), m_y(n), m_z(n) {}

    inline constexpr Vec3d operator+(const Vec3d &other) const
    {
        return Vec3d{m_x + other.m_x, m_y + other.m_y, m_z + other.m_z};
    }
    inline constexpr Vec3d &operator+=(const Vec3d &other)
    {
        m_x += other.m_x;
        m_y += other.m_y;
        m_z += other.m_z;
        return *this;
    }
    inline constexpr Vec3d operator-(const Vec3d &other) const
    {
        return Vec3d{m_x - other.m_x, m_y - other.m_y, m_z - other.m_z};
    }
    inline constexpr Vec3d &operator-=(const Vec3d &other)
    {
        m_x -= other.m_x;
        m_y -= other.m_y;
        m_z -= other.m_z;
        return *this;
    }
    inline constexpr Vec3d operator*(double scalar) const
    {
        return Vec3d{m_x * scalar, m_y * scalar, m_z * scalar};
    }
    inline constexpr Vec3d &operator*=(double scalar)
    {
        m_x *= scalar;
        m_y *= scalar;
        m_z *= scalar;
        return *this;
    }
    inline constexpr Vec3d operator/(double scalar) const
    {
        return Vec3d{m_x / scalar, m_y / scalar, m_z / scalar};
    }
    inline constexpr Vec3d &operator/=(double scalar)
    {
        m_x /= scalar;
        m_y /= scalar;
        m_z /= scalar;
        return *this;
    }
    inline constexpr double operator*(const Vec3d &other) const
    {
        return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z;
    }

    friend inline constexpr Vec3d operator*(double scalar, const Vec3d &v)
    {
        return v * scalar;
    }
    friend inline constexpr Vec3d operator+(double scalar, const Vec3d &v)
    {
        return Vec3d{v.m_x + scalar, v.m_y + scalar, v.m_z + scalar};
    }
#endif
    inline double length2(double softening = 0.0) const
    {
        return (x() * x() + y() * y() + z() * z()) + softening * softening;
    }
    inline double length(double softening = 0.0) const
    {
        return std::sqrt((x() * x() + y() * y() + z() * z()) +
                         softening * softening);
    }
    inline Vec3d normalized(double softening = 0.0) const
    {
        return (softening + *this) / this->length(softening);
    }

    explicit operator glm::vec3() const { return glm::vec3(x(), y(), z()); }

    template <typename T> T operator+(const T &other) const
    {
        return T{static_cast<decltype(other.x)>(x) + other.x,
                 static_cast<decltype(other.y)>(y) + other.y,
                 static_cast<decltype(other.z)>(z) + other.z};
    }
};

#ifdef INTEL_AVX2
inline double operator*(const Vec3d &a, const Vec3d &b)
{
    __m256d va = _mm256_loadu_pd(a.v);
    __m256d vb = _mm256_loadu_pd(b.v);
    __m256d mul = _mm256_mul_pd(va, vb);

    __m128d lo = _mm256_castpd256_pd128(mul);
    __m128d hi = _mm256_extractf128_pd(mul, 1);

    __m128d sum2 = _mm_add_pd(lo, hi);
    __m128d swapped = _mm_permute_pd(sum2, 1);
    __m128d dot = _mm_add_sd(sum2, swapped);

    return _mm_cvtsd_f64(dot);
}
#else
inline double operator*(const Vec3d &a, const Vec3d &b)
{
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}
#endif

struct ImGuiWindowInfo
{
    ImVec2 size;
    ImVec2 pos;
};

enum class OdeMethod
{
    RK4 = 1,
    ForwardEuler,
    Leapfrog
};
} // namespace Types

namespace Constants
{
namespace
{
constexpr const uint32_t WIDTH = 1280;
constexpr const uint32_t HEIGHT = 720;
constexpr const uint32_t TEXTURE_COUNT = 0;
constexpr const float AXES_LENGTH = 2000.0f;
constexpr const float EPSILON = 0.0000000000000000000000001f;
constexpr const glm::vec3 WORLD_UP = glm::vec3(0.0f, -1.0f, 0.0f);

const char *methods[] = {"1. RK4", "2. ForwardEuler", "3. Leapfrog"};

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::filesystem::path SHADERS_PATH = "shaders";
const std::filesystem::path TEXTURES_PATH = "textures";
const std::filesystem::path MODELS_PATH = "models";
const std::filesystem::path FONTS_PATH = "fonts";

template <typename T> constexpr T unitPrefixFactor(const char prefix)
{
    switch (prefix)
    {
    case 'n':
        return 1e-9;
    // case 'μ': return 1e-6;
    case 'm':
        return 1e-3;
    case 'k':
        return 1e3;
    default:
        return 1.0;
    }
}
} // namespace
} // namespace Constants

namespace Helpers
{
inline std::string vectorFormat(const Types::Vec3d &vec)
{
    return std::format("({:.2f}, {:.2f}, {:.2f})", vec.x(), vec.y(), vec.z());
}

inline std::vector<char> readFile(const std::filesystem::path &filepath)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file: " + filepath.string());
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                               uint32_t typeFilter,
                               VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

inline VkCommandBuffer beginSingleTimeCommands(VkDevice device,
                                               VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

inline void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue,
                                  VkCommandPool commandPool,
                                  VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

inline void copyBuffer(VkDevice device, VkQueue graphicsQueue,
                       VkCommandPool commandPool, VkBuffer srcBuffer,
                       VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer =
        beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.dstOffset = 0;
    copyRegion.srcOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
}

inline VkShaderModule createShaderModule(VkDevice device,
                                         const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}
} // namespace Helpers
