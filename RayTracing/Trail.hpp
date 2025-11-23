#pragma once

#include "Node.hpp"

class Trail : public Node
{
	int idx = 0;
	glm::vec3 m_initialPos;
	glm::vec3 m_lastPos;
public:
	Trail(glm::vec3 startPos) : m_initialPos(startPos), m_lastPos(startPos) {}
	void setInitialPos(glm::vec3 pos) { m_initialPos = pos; }
	void setLastPos(glm::vec3 pos) { m_lastPos = pos; }
	void addSectionRed(glm::vec3 from, glm::vec3 to);
	void addSectionRed(glm::vec3 to);
	void addSectionYellow(glm::vec3 from, glm::vec3 to);
	void addSectionYellow(glm::vec3 to);
	void addSectionGreen(glm::vec3 from, glm::vec3 to);
	void addSectionGreen(glm::vec3 to);
	void cleanup() override;
};