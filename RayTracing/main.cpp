#include "Simulation.h"
#include "VulkanRenderer.h"
#include "Node.h"

int main() {
	VulkanRenderer renderer;
	Node::setRenderer(&renderer);
    Simulation app(renderer);

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}