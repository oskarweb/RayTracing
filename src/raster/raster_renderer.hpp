#pragma once

#include "core/camera/camera.hpp"
#include "core/extras.hpp"
#include "core/resources/mesh.hpp"
#include "core/resources/vertex.hpp"
#include "core/scene/models.hpp"
#include "core/scene/renderer.hpp"
#include "core/scene/renderer_extras.hpp"
#include "core/sparse_set.hpp"
#include "core/vulkan/vulkan_handles.hpp"

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

[[nodiscard]] constexpr bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

inline constexpr std::array deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

class VulkanRenderer : public Renderer
{
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    VulkanRenderer() = default;
    ~VulkanRenderer() override;

    void setCamera(Camera *camera) { m_cameraPtr = camera; }
    void setWindow(GLFWwindow *window) { m_window = window; }

    void init();
    void newFrame();
    void recordImguiData(ImDrawData *data);
    SparseSet<Material>::Handle getMaterial(const std::string &name) override;
    SparseSet<Mesh>::Handle getMesh(const std::string &name) override;
    SparseSet<RenderObject>::Handle addRenderObject(RenderObject obj) override;
    void removeRenderObject(SparseSet<RenderObject>::Handle handle) override;
    void cleanup();
    [[nodiscard]] int getFramebufferWidth() const noexcept { return m_framebufferWidth; }
    [[nodiscard]] int getFramebufferHeight() const noexcept { return m_framebufferHeight; }
    [[nodiscard]] double getDeltaTime() const noexcept { return m_deltaTime; }
    [[nodiscard]] double getDeltaTimeS() const noexcept { return m_deltaTime; }
    void notifyFramebufferResized() { m_framebufferResized = true; }
    std::string createParaboloid(std::string meshName, int nx, int nz,
                                 const std::function<double(double, double)> &formula);
    RenderObject *getRenderObject(SparseSet<RenderObject>::Handle handle) override;
    Ray castRayIntoWorld(float x, float y);

private:
    bool m_imguiInitialized = false;
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
    void createMaterials();
    void createDescriptorSetLayout();
    void createFramebuffers();
    void createCommandPool();
    void createColorResources();
    void createDepthResources();
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createTextureImage(Texture &texture, const std::filesystem::path &path);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    void createTextureImageView(vk::raii::ImageView &textureImageView, VkImage &textureImage);
    void createTextureSampler();
    vk::raii::ImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                        uint32_t mipLevels);
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

    vk::raii::Context m_context;
    vk::raii::Instance m_instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};
    vk::raii::SurfaceKHR m_surface{nullptr};
    vk::raii::PhysicalDevice m_physicalDevice{nullptr};
    vk::raii::Device m_device{nullptr};
    Allocator m_allocator;
    vk::raii::DescriptorSetLayout m_descriptorSetLayout{nullptr};
    vk::raii::RenderPass m_renderPass{nullptr};

    SparseSet<RenderObject> m_renderObjects;
    SparseSet<Mesh> m_meshes;
    SparseSet<Material> m_materials;
    std::vector<Texture> m_textures{};

    std::unordered_map<std::string, SparseSet<Mesh>::Handle> m_meshHandles{};
    std::unordered_map<std::string, SparseSet<Material>::Handle> m_materialHandles{};

    Camera *m_cameraPtr = nullptr;
    GLFWwindow *m_window = nullptr;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    vk::raii::SwapchainKHR m_swapChain{nullptr};
    vk::raii::SwapchainKHR m_prevSwapChain{nullptr};
    std::vector<VkImage> m_swapChainImages{};
    VkFormat m_swapChainImageFormat{};
    VkExtent2D m_swapChainExtent{};
    std::vector<vk::raii::ImageView> m_swapChainImageViews{};
    bool m_framebufferResized = false;

    vk::raii::CommandPool m_commandPool{nullptr};
    std::vector<vk::raii::CommandBuffer> m_commandBuffers{};

    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores{};
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores{};
    std::vector<vk::raii::Fence> m_inFlightFences{};
    std::vector<VkFence> m_imagesInFlight{};

    uint32_t m_currentFrame = 0;

    std::vector<AllocatedBuffer> m_uniformBuffers{};
    vk::raii::DescriptorPool m_descriptorPool{nullptr};
    std::vector<vk::raii::DescriptorSet> m_descriptorSets{};

    CameraBuffer m_cameraUBO{};

    uint32_t m_mipLevels = 0;
    vk::raii::Sampler m_textureSampler{nullptr};

    Image m_depthImage{};
    vk::raii::ImageView m_depthImageView{nullptr};
    Image m_colorImage{};
    vk::raii::ImageView m_colorImageView{nullptr};
    std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers{};

    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    int m_framebufferWidth = Constants::WIDTH;
    int m_framebufferHeight = Constants::HEIGHT;

    vk::raii::DescriptorPool m_imguiDescriptorPool{nullptr};
    std::vector<ImDrawData *> m_imguiDrawData{};

    std::chrono::steady_clock::time_point m_lastFrameTime{};
    double m_deltaTime = 0.0;
};
