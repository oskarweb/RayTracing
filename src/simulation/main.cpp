#include "simulation.hpp"

#include "node.hpp"
#include "vulkan_renderer.hpp"

int main()
{
    VulkanRenderer renderer{};
    Node::setRenderer(&renderer);
    Simulation app(&renderer);

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}