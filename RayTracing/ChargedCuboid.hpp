#pragma once

#include "Node.hpp"
#include "extras.hpp"

class ChargedCuboid : public Node
{
public:
	inline static const std::string& C_MODEL_NAME = "cuboid";

	ChargedCuboid(double charge, Types::Vec3d pos, Types::Vec3d shape) :
		m_charge(charge),
		m_pos(pos),
		m_shape(shape)
	{
		uploadModel(C_MODEL_NAME, std::make_unique<CuboidModel>(static_cast<glm::vec3>(m_pos), static_cast<glm::vec3>(m_shape)));
	}

	void cleanup() override;

private:
	Types::Vec3d m_pos;
	Types::Vec3d m_shape;
	double m_charge;
};