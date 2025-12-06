#include "trail.hpp"

void Trail::addSectionRed(glm::vec3 from, glm::vec3 to)
{
    uploadModel(std::to_string(idx), std::make_unique<RedLineModel>(from, to));
    idx++;
}

void Trail::addSectionRed(glm::vec3 to)
{
    uploadModel(std::to_string(idx),
                std::make_unique<RedLineModel>(m_lastPos, to));
    idx++;
    m_lastPos = to;
}

void Trail::addSectionYellow(glm::vec3 from, glm::vec3 to)
{
    uploadModel(std::to_string(idx),
                std::make_unique<YellowLineModel>(from, to));
    idx++;
}

void Trail::addSectionYellow(glm::vec3 to)
{
    uploadModel(std::to_string(idx),
                std::make_unique<YellowLineModel>(m_lastPos, to));
    idx++;
    m_lastPos = to;
}

void Trail::addSectionGreen(glm::vec3 from, glm::vec3 to)
{
    uploadModel(std::to_string(idx),
                std::make_unique<GreenLineModel>(from, to));
    idx++;
}

void Trail::addSectionGreen(glm::vec3 to)
{
    uploadModel(std::to_string(idx),
                std::make_unique<GreenLineModel>(m_lastPos, to));
    idx++;
    m_lastPos = to;
}

void Trail::cleanup()
{
    Node::cleanup();
    m_models.clear();
    m_lastPos = m_initialPos;
    idx = 0;
}