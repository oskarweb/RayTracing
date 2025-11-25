#pragma once

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "imgui.h"

#include <fstream>
#include <cstdint>
#include <vector>
#include <filesystem>
#include <map>
#include <format>

namespace Types
{
    struct Vec3d
    {
        double x;
        double y;
        double z;
    
        constexpr Vec3d() : x(0.0), y(0.0), z(0.0) {}
        constexpr Vec3d(double x, double y, double z) : x(x), y(y), z(z) {}
        constexpr Vec3d(double n) : x(n), y(n), z(n) {}
    
        inline constexpr double length2(double softening = 0.0) const { return (x * x + y * y + z * z) + softening * softening; }
        inline double length(double softening = 0.0) const { return std::sqrt((x * x + y * y + z * z) + softening * softening); }
        inline Vec3d normalized(double softening = 0.0) const { return (softening + *this) / this->length(softening); }
    
        inline constexpr Vec3d operator+(const Vec3d& other) const   { return Vec3d{ x + other.x, y + other.y, z + other.z }; }
        inline constexpr Vec3d& operator+=(const Vec3d& other)       { x += other.x; y += other.y; z += other.z; return *this; }
        inline constexpr Vec3d operator-(const Vec3d& other) const   { return Vec3d{ x - other.x, y - other.y, z - other.z }; }
        inline constexpr Vec3d& operator-=(const Vec3d& other)       { x -= other.x; y -= other.y; z -= other.z; return *this; }
        inline constexpr Vec3d operator*(double scalar) const        { return Vec3d{ x * scalar, y * scalar, z * scalar }; }
        inline constexpr Vec3d& operator*=(double scalar)            { x *= scalar; y *= scalar; z *= scalar; return *this; }
        inline constexpr Vec3d operator/(double scalar) const        { return Vec3d{ x / scalar, y / scalar, z / scalar }; }
        inline constexpr Vec3d& operator/=(double scalar)            { x /= scalar; y /= scalar; z /= scalar; return *this; }
		inline constexpr Vec3d operator*(const Vec3d& other) const   { return Vec3d{ x * other.x, y * other.y, z * other.z }; }
    
        explicit operator glm::vec3() const { return glm::vec3(x, y, z); }
    
        template<typename T>
        T operator+(const T& other) const
        {
            return T{ static_cast<decltype(other.x)>(x) + other.x, static_cast<decltype(other.y)>(y) + other.y, static_cast<decltype(other.z)>(z) + other.z };
        }

		friend inline constexpr Vec3d operator*(double scalar, const Vec3d& v) { return v * scalar; }
        friend inline constexpr Vec3d operator+(double scalar, const Vec3d& v) { return Vec3d{ v.x + scalar, v.y + scalar, v.z + scalar }; }
    };

	inline constexpr double operator*(const Vec3d& v) { return v.x + v.y + v.z; }

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
}

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

    const char* methods[] = { "1. RK4", "2. ForwardEuler", "3. Leapfrog"};

	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::filesystem::path SHADERS_PATH = "shaders";
	const std::filesystem::path TEXTURES_PATH = "textures";
	const std::filesystem::path MODELS_PATH = "models";
    const std::filesystem::path FONTS_PATH = "fonts";

    template<typename T>
    constexpr T unitPrefixFactor(const char prefix)
    {
		switch(prefix)
		{
		    case 'n': return 1e-9;
		    //case 'μ': return 1e-6;
		    case 'm': return 1e-3;
	    	case 'k': return 1e3;
		    default: return 1.0;
		}
    }
}
}

namespace Helpers
{
	inline std::string vectorFormat(const Types::Vec3d& vec)
	{
		return std::format("({:.2f}, {:.2f}, {:.2f})", vec.x, vec.y, vec.z);
	}

    inline std::vector<char> readFile(const std::filesystem::path& filepath)
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

    inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    inline VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
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

    inline void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    inline void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }

    inline VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }
}
