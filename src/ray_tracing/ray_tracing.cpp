#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "implot.h"

#include <cstring>

void RayTracing::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(Constants::WIDTH, Constants::HEIGHT, "Simulation", nullptr, nullptr);
    glfwSetWindowSizeLimits(m_window, Constants::WIDTH, Constants::HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void RayTracing::addParticle(Particle &&particle)
{
    auto &particleRef = m_particles.emplace_back(std::move(particle));
    m_plotSelectedParticle = m_particles.begin();
}

void RayTracing::removeParticle(std::vector<Particle>::iterator &it)
{
    if (it != m_particles.end())
    {
        (*it).cleanup();
        m_particles.erase(it);
        if (m_particles.size() > 0)
            m_plotSelectedParticle = m_particles.begin();
    }
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
    m_rendererHandle->addRenderables(&axes);

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

        if (not m_paused)
        {
            if (m_skipUpdate)
            {
                m_skipUpdate = false;
            }
            else
            {
                switch (m_mode)
                {
                case SimulationMode::Static:
                    updateStatic();
                    break;
                case SimulationMode::PrecalculatedAll:
                    updateAllPrecalc();
                    break;
                case SimulationMode::Precalculated20Ms:
                    update20MsPecalc();
                    break;
                case SimulationMode::Realtime:
                    updateRealTime();
                    break;
                }
            }
        }

        displayMainCtrlWindow();
        displayParticleListWindow();
        displayParticleAddWindow();
        displayPlotWindow();

        ImGui::Render();
        m_rendererHandle->recordImguiData(ImGui::GetDrawData());
    }

    m_rendererHandle->cleanup();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}