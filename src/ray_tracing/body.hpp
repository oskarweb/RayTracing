#pragma once

#include "extras.hpp"
#include "node.hpp"

class Body : public Node
{
public:
    Body(Types::Vec3d pos, Types::Vec3d velocity);
    void update(double dt);
    void cleanup() override { Node::cleanup(); }

private:
    Types::Vec3d m_pos;
    Types::Vec3d m_velocity;
    Types::Vec3d m_acceleration;
    Types::Vec3d m_affectingForce;
};