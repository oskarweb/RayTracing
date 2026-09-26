#include "raster/raster_renderer.hpp"

#include <filesystem>
#include <stdexcept>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<AllocatedBuffer>);
static_assert(!std::is_copy_constructible_v<Image>);
static_assert(!std::is_copy_constructible_v<Material>);
static_assert(!std::is_copy_constructible_v<Mesh>);
static_assert(std::is_nothrow_move_constructible_v<AllocatedBuffer>);
static_assert(std::is_nothrow_move_constructible_v<Image>);

namespace
{
void renderFrames(VulkanRenderer &renderer, int count)
{
    for (int frame = 0; frame < count; ++frame)
    {
        glfwPollEvents();
        renderer.newFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Lifetime smoke test");
        ImGui::TextUnformatted("Rendering after resource ownership migration");
        ImGui::End();
        ImGui::Render();
        renderer.recordImguiData(ImGui::GetDrawData());
    }
}
} // namespace

int main()
{
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto window = glfwCreateWindow(1280, 720, "Renderer lifetime smoke test", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    int result = 0;
    try
    {
        Camera camera;
        // Missing shaders fail after instance/device/swapchain creation. Their
        // owners must unwind before another renderer can use the same window.
        const auto originalDirectory = std::filesystem::current_path();
        const auto emptyDirectory = originalDirectory / "missing-shaders-smoke";
        std::filesystem::create_directories(emptyDirectory);
        bool failedAtShaders = false;
        std::filesystem::current_path(emptyDirectory);
        try
        {
            VulkanRenderer renderer;
            renderer.setWindow(window);
            renderer.setCamera(&camera);
            renderer.init();
        }
        catch (const std::exception &error)
        {
            failedAtShaders = std::string(error.what()).find("failed to open file:") != std::string::npos;
        }
        std::filesystem::current_path(originalDirectory);
        if (!failedAtShaders)
            throw std::runtime_error("Expected initialization to fail at shader loading");

        for (int pass = 0; pass < 2; ++pass)
        {
            VulkanRenderer renderer;
            renderer.setWindow(window);
            renderer.setCamera(&camera);
            renderer.init();
            renderer.addRenderObject({renderer.getMesh("cube"), renderer.getMaterial("cube"), glm::mat4(1.0f)});
            renderer.addRenderObject({renderer.getMesh("xAxis"), renderer.getMaterial("line"), glm::mat4(1.0f)});
            renderer.addRenderObject({renderer.getMesh("cube"), renderer.getMaterial("paraboloid"), glm::mat4(1.0f)});
            renderFrames(renderer, 6);
            glfwSetWindowSize(window, 1440, 900);
            renderer.notifyFramebufferResized();
            renderFrames(renderer, 6);
            glfwSetWindowSize(window, 1280, 720);
            renderer.notifyFramebufferResized();
            renderFrames(renderer, 6);
            // Cover both explicit (including repeated) and destructor cleanup.
            if (pass == 0)
            {
                renderer.cleanup();
                renderer.cleanup();
                renderer.init();
                renderer.addRenderObject({renderer.getMesh("cube"), renderer.getMaterial("cube"), glm::mat4(1.0f)});
                renderFrames(renderer, 4);
                if (renderer.getDeltaTime() != renderer.getDeltaTimeS())
                    throw std::runtime_error("Frame timing units disagree");
                renderer.cleanup();
            }
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
