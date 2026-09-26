#pragma once

#include "core/extras.hpp"
#include "core/scene/node.hpp"

#include <string>

class Surface : public Node
{
public:
    Surface(const std::string &meshName, Types::Vec3d pos);
    void cleanup() override { Node::cleanup(); }

private:
    Types::Vec3d m_pos;
};
