#include "raytracing/surface.hpp"

Surface::Surface(const std::string &meshName, Types::Vec3d pos) : m_pos(pos)
{
    uploadModel("surface", std::make_unique<ParaboloidModel>(static_cast<glm::vec3>(m_pos), glm::vec3(1.0f), meshName));
}
