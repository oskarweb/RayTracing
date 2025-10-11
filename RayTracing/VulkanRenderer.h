#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "tiny_obj_loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "implot.h"

#include <iostream>
#include <unordered_map>
#include <array>
#include <vector>
#include <filesystem>
#include <utility>
#include <memory>
#include <set>

#include "Renderer.h"
#include "extras.h"
#include "Camera.h"
#include "Vertex.h"
#include "Mesh.h"
#include "PipelineFactory.h"
#include "RendererStructs.h"
#include "Models.h"

class VulkanRenderer : public Renderer
{
public:
	const static int MAX_FRAMES_IN_FLIGHT = 2;

	void setCamera(Camera* camera)
	{
		m_cameraPtr = camera;
	}

	void setWindow(GLFWwindow* window)
	{
		m_window = window;
		glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
	}

	void init();
	void newFrame();
	void recordImguiData(ImDrawData* data);
	Material* getMaterial(const std::string& name);
	Mesh* getMesh(const std::string& name);
	std::multimap<std::string, Renderable, RenderableComp>::iterator addRenderable(Renderable renderable) override;
	void addRenderables(Model* model) override;
	void removeRenderable(std::multimap<std::string, Renderable, RenderableComp>::iterator& it) override;
	void cleanup();
	const int& getFramebufferWidth() const { return m_framebufferWidth; }
	const int& getFramebufferHeight() const { return m_frameBufferheight; }
	const double& getDeltaTime() const { return m_deltaTime; }
	double getDeltaTimeS() const { return m_deltaTime / 1000.0; }
private:
	void initImgui();
	void cleanupSwapChain();
	void recreateSwapChain();
	void createInstance();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createFramebuffers();
	void createCommandPool();
	void createColorResources();
	void createDepthResources();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	void createTextureImage(Texture& texture, std::filesystem::path& path);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	void createTextureImageView(VkImageView& textureImageView, VkImage& textureImage);
	void createTextureSampler();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Image& image);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createCommandBuffers();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects();
	void updateUniformBuffer(uint32_t currentImage);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void drawObjects(VkCommandBuffer& commandBuffer);

	//VARS
	std::multimap<std::string, Renderable, RenderableComp> m_renderableObjects{};
	uint64_t m_currentRenderableId = 0;
	std::unordered_map<std::string, Mesh> m_meshes{};
	std::unordered_map<std::string, Material> m_materials{};
	std::vector<Texture> m_textures{};

	std::vector<Mesh> testMesh;

	Camera* m_cameraPtr = nullptr;
	GLFWwindow* m_window = nullptr;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages{};
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImageView> m_swapChainImageViews{};
	std::vector<VkFramebuffer> m_swapChainFramebuffers{};

	VkRenderPass m_renderPass;
	VkDescriptorSetLayout m_descriptorSetLayout;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers{};

	std::vector<VkSemaphore> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores{};
	std::vector<VkFence> m_inFlightFences{};

	uint32_t m_currentFrame = 0;

	std::vector<VkBuffer> m_uniformBuffers{};
	std::vector<VkDeviceMemory> m_uniformBuffersMemory{};
	std::vector<void*> m_uniformBuffersMapped{};

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;

	uint32_t m_mipLevels;
	VkSampler m_textureSampler;

	Image m_depthImage;
	VkImageView m_depthImageView;

	Image m_colorImage;
	VkImageView m_colorImageView;

	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	int m_framebufferWidth = Constants::WIDTH;
	int m_frameBufferheight = Constants::HEIGHT;
	bool m_framebufferResized = false;

	VkDescriptorPool m_imguiDescriptorPool;
	std::array<ImDrawData*, MAX_FRAMES_IN_FLIGHT> m_imguiDrawData{nullptr};

	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime{};
	double m_deltaTime = 0.0;
};

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

inline bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};