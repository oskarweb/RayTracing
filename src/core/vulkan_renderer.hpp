#pragma once

#include "camera.hpp"
#include "extras.hpp"
#include "mesh.hpp"
#include "models.hpp"
#include "pipeline_factory.hpp"
#include "renderer.hpp"
#include "renderer_extras.hpp"
#include "sparse_set.hpp"
#include "vertex.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <array>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T *;

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger)
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

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator)
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

const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

class VulkanRenderer : public Renderer
{
public:
    constexpr const static int MAX_FRAMES_IN_FLIGHT = 3;

    VulkanRenderer() = default;

    void setCamera(Camera *camera) { m_cameraPtr = camera; }
    void setWindow(GLFWwindow *window) { m_window = window; }

    void init();
    void newFrame();
    void recordImguiData(ImDrawData *data);
    SparseSet<Material>::Handle getMaterial(const std::string &name) override;
    SparseSet<Mesh>::Handle getMesh(const std::string &name) override;
    SparseSet<RenderObject>::Handle addRenderObject(RenderObject obj) override;
    // void addRenderables(Model *model) override;
    void removeRenderObject(SparseSet<RenderObject>::Handle handle) override;
    void cleanup();
    const int &getFramebufferWidth() const { return m_framebufferWidth; }
    const int &getFramebufferHeight() const { return m_frameBufferHeight; }
    const double &getDeltaTime() const { return m_deltaTime; }
    double getDeltaTimeS() const { return m_deltaTime / 1000.0; }
    void notifyFramebufferResized() { m_framebufferResized = true; }
    std::string createParaboloid(std::string meshName, int nx, int nz,
                                 const std::function<double(double, double)> &formula);
    RenderObject *getRenderObject(SparseSet<RenderObject>::Handle handle) override;
    Ray castRayIntoWorld(float x, float y);

private:
    void initImplVulkanImGui();
    void initImgui();
    void cleanupSyncObjects();
    void cleanupSwapChain(bool recreate);
    void recreateSwapChain();
    void createAllocator();
    void createInstance();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
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
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createTextureImage(Texture &texture, std::filesystem::path &path);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    void createTextureImageView(VkImageView &textureImageView, VkImage &textureImage);
    void createTextureSampler();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                     VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     Image &image);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createSyncObjects();
    void updateUniformBuffer(uint32_t currentImage);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    std::vector<const char *> getRequiredExtensions();
    bool checkValidationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData);
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    void drawObjects(VkCommandBuffer &commandBuffer);

    SparseSet<RenderObject> m_renderObjects;
    SparseSet<Mesh> m_meshes;
    SparseSet<Material> m_materials;
    std::vector<Texture> m_textures{};

    std::unordered_map<std::string, SparseSet<Mesh>::Handle> m_meshHandles{};
    std::unordered_map<std::string, SparseSet<Material>::Handle> m_materialHandles{};

    std::vector<Mesh> testMesh{};

    Camera *m_cameraPtr = nullptr;
    GLFWwindow *m_window = nullptr;

    VmaAllocator m_allocator{};

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkSwapchainKHR m_prevSwapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapChainImages{};
    VkFormat m_swapChainImageFormat{};
    VkExtent2D m_swapChainExtent{};
    std::vector<VkImageView> m_swapChainImageViews{};
    std::vector<VkFramebuffer> m_swapChainFramebuffers{};
    bool m_framebufferResized = false;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers{};

    std::vector<VkSemaphore> m_imageAvailableSemaphores{};
    std::vector<VkSemaphore> m_renderFinishedSemaphores{};
    std::vector<VkFence> m_inFlightFences{};
    std::vector<VkFence> m_imagesInFlight{};

    uint32_t m_currentFrame = 0;

    std::vector<VkBuffer> m_uniformBuffers{};
    std::vector<VmaAllocation> m_uniformBuffersAlloc{};
    std::vector<void *> m_uniformBuffersMapped{};
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptorSets{};

    CameraBuffer m_cameraUBO{};

    uint32_t m_mipLevels = 0;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    Image m_depthImage{};
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    Image m_colorImage{};
    VkImageView m_colorImageView = VK_NULL_HANDLE;

    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    int m_framebufferWidth = Constants::WIDTH;
    int m_frameBufferHeight = Constants::HEIGHT;

    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    std::vector<ImDrawData *> m_imguiDrawData{};

    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime{};
    double m_deltaTime = 0.0;
};