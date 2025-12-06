#pragma once

#include "camera.hpp"
#include "extras.hpp"
#include "input.hpp"
#include "vulkan_renderer.hpp"

class RayTracing
{
public:
    RayTracing(VulkanRenderer *rendererHandle)
        : m_rendererHandle(rendererHandle),
          m_camera(Camera(glm::vec3(-10.0f, -10.0f, -10.0f), 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)))
    {
    }

    void run();

private:
    static inline void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    void initWindow();

    GLFWwindow *m_window = nullptr;
    VulkanRenderer *m_rendererHandle;
    Camera m_camera;
};