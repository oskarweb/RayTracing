#pragma once

#include "models.hpp"
#include "renderer.hpp"

class Node : public BaseObject
{
public:
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;
    Node(Node &&) noexcept = default;
    Node &operator=(Node &&) noexcept = default;
    Node() = default;

    void uploadModel(const std::string &name, std::unique_ptr<Model> model);
    Model *getModel(const std::string &name);
    virtual void cleanup() = 0;

protected:
    std::map<std::string, std::unique_ptr<Model>> m_models{};
};
