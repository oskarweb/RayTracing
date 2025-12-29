#include "ray_tracing.hpp"
#include "body.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "implot.h"

#include <cstring>

void RayTracing::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    RayTracing *app = static_cast<RayTracing *>(glfwGetWindowUserPointer(window));
    app->m_rendererHandle->notifyFramebufferResized();
}

void RayTracing::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(Constants::WIDTH, Constants::HEIGHT, "RayTracing", nullptr, nullptr);
    glfwSetWindowSizeLimits(m_window, Constants::WIDTH, Constants::HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void RayTracing::run()
{
    initWindow();
    Input::setWindow(m_window);
    m_rendererHandle->setWindow(m_window);
    m_rendererHandle->setCamera(&m_camera);
    m_rendererHandle->init();
    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwMakeContextCurrent(m_window);

    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    std::filesystem::path arimoPath = Constants::FONTS_PATH / "arimo" / "Arimo-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(arimoPath.string().c_str(), 16.0f, &config, io.Fonts->GetGlyphRangesGreek());
    io.Fonts->Build();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_CursorPosCallback(m_window, Input::mousePos.x, Input::mousePos.y);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    AxesModel axes(glm::vec3(0.0f));
    const std::string &paraboloidMeshName = m_rendererHandle->createParaboloid(
        "paraboloid", 20, 20, [](double x, double y) -> double { return y * y / 2 - x * x / 3; });
    const std::string &paraboloidMeshName2 = m_rendererHandle->createParaboloid(
        "paraboloid", 20, 20, [](double x, double y) -> double { return y * y / 0.1 - x * x / 0.01; });
    ParaboloidModel plane(glm::vec3(0.0f), glm::vec3(10.0f, 3.0f, 10.0f), paraboloidMeshName);
    ParaboloidModel plane2(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(10.0f, 3.0f, 10.0f), paraboloidMeshName2);

    // Body body{Types::Vec3d(0.0, -5.0, 0.0), Types::Vec3d(1.0, 0.0, 1.0)};

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        m_camera.update(static_cast<float>(m_rendererHandle->getDeltaTime()));

        m_rendererHandle->newFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImPlot::ShowDemoWindow();
        ImGui::ShowDemoWindow();

        ImGui::Render();
        m_rendererHandle->recordImguiData(ImGui::GetDrawData());
    }

    m_rendererHandle->cleanup();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}