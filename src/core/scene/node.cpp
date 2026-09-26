#include "core/scene/node.hpp"

void Node::cleanup()
{
    for (auto &[name, model] : m_models)
    {
        model->cleanup();
    }
}

void Node::uploadModel(const std::string &name, std::unique_ptr<Model> model)
{
    // BaseObject::rendererHandle->addRenderables(model.get());
    if (!model)
        throw std::invalid_argument("Cannot upload a null model: " + name);
    m_models.insert_or_assign(name, std::move(model));
}

Model *Node::getModel(const std::string &name) { return m_models.at(name).get(); }
