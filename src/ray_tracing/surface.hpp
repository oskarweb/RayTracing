#pragma once

#include "extras.hpp"
#include "node.hpp"

#include <string>

class Surface : public Node
{
public:
    Surface(const std::string &meshName, Types::Vec3d pos);

private:
    Types::Vec3d m_pos;
};