#include "body.hpp"

Body::Body(Types::Vec3d pos, Types::Vec3d velocity) : m_pos(pos), m_velocity(velocity)
{
    uploadModel("body", std::make_unique<ParticleModel>(static_cast<glm::vec3>(m_pos)));
}

void Body::update(double dt) { m_models["body"]->update(static_cast<glm::vec3>(m_pos)); }