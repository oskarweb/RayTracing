#include "surface.hpp"

Surface::Surface(const std::string &meshName, Types::Vec3d pos) : m_pos(pos)
{
    uploadModel("body", std::make_unique<ParticleModel>(static_cast<glm::vec3>(m_pos)));
}