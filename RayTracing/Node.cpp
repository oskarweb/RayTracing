#include "Node.hpp"

void Node::setRenderer(Renderer *renderer)
{
	rendererHandle = renderer;
}

void Node::cleanup()
{
	for (auto& [name, model] : m_models)
	{
		model->cleanup(rendererHandle);
	}
}

void Node::uploadModel(const std::string& name, std::unique_ptr<Model> model)
{
	rendererHandle->addRenderables(model.get());
	m_models.insert({ name, std::move(model) });
}

Model* Node::getModel(const std::string& name)
{
	return m_models[name].get();
}