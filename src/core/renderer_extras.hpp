#pragma once 

#include "mesh.hpp"

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;

struct Material
{
	std::string name = "default";
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	Material() : pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {}
	Material(const std::string& name, VkPipeline pipeline, VkPipelineLayout pipelineLayout) : name(name), pipeline(pipeline), pipelineLayout(pipelineLayout) {}
};

struct Renderable
{
	Mesh* mesh = nullptr;
	Material* material = nullptr;
    glm::mat4 transformMatrix{};

	std::unique_ptr<int> textureIdx = nullptr;
	uint64_t id = 0;
};

struct RenderableComp
{
    bool operator()(const std::string& lhs, const std::string& rhs) const
    {
        return lhs < rhs;
    }
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

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

struct CameraBuffer {
	glm::mat4 view{};
	glm::mat4 proj{};
};

struct RenderableInfo
{
	RenderableInfo(
        const std::string& renderableName,
        const std::string& meshName,
        const std::string& materialName,
        glm::mat4 transformMatrix
    ) : renderableName(renderableName), meshName(meshName), materialName(materialName), transformMatrix(transformMatrix) 
    {}

    std::string renderableName{};
    std::string meshName{};
    std::string materialName{};
    glm::mat4 transformMatrix{};
};

inline constexpr const std::array<Vertex, 36> cubeVertices = {
    // Front
    Vertex{{-0.5f, 0.5f,-0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,-0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f,-0.5f,-0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
          
    Vertex{{-0.5f,-0.5f,-0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,-0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f,-0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    // Back  
    Vertex{{ 0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f,-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
          
    Vertex{{-0.5f,-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    // Left    
    Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f,-0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
         
    Vertex{{-0.5f,-0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f,-0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Right  
    Vertex{{ 0.5f, 0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
         
    Vertex{{ 0.5f,-0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Top    
    Vertex{{-0.5f,-0.5f,-0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f,-0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f,-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        
    Vertex{{-0.5f,-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f,-0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f,-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    // Bottom
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,-0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f,-0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
          
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,-0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 18> pyramidVertices = {
    // Front
    Vertex{{-0.5f, 0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,-0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.0f,-0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Back          
    Vertex{{ 0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.0f,-0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Left          
    Vertex{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f,-0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.0f,-0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    // Right         
    Vertex{{ 0.5f, 0.5f,-0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.0f,-0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    // Bottom
    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    Vertex{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{-0.5f, 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.5f, 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> redLineVertices = {
    Vertex{{ 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> yellowLineVertices = {
    Vertex{{ 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> greenLineVertices = {
    Vertex{{ 0.0f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 1.0f, 0.0f, 0.0f}, {0.0f, 0.3f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> xAxisVertices = {
    Vertex{{ 0.0f,                   0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ Constants::AXES_LENGTH, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> yAxisVertices = {
    Vertex{{ 0.0f, 0.0f,                   0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.0f, Constants::AXES_LENGTH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
};

inline constexpr const std::array<Vertex, 2> zAxisVertices = {
    Vertex{{ 0.0f, 0.0f,                   0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    Vertex{{ 0.0f, 0.0f, Constants::AXES_LENGTH}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
};