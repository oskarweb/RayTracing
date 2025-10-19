#pragma once

#include "Models.hpp"
#include "Renderer.hpp"

class Node
{
public:
	Node(const Node&) = delete;
	Node& operator=(const Node&) = delete;
	Node(Node&&) noexcept = default;
	Node& operator=(Node&&) noexcept = default;
	Node() = default;

	static void setRenderer(Renderer* renderer);
	void uploadModel(const std::string& name, std::unique_ptr<Model> model);
	Model* getModel(const std::string& name);
    virtual void cleanup() = 0;
protected:
	std::map<std::string, std::unique_ptr<Model>> m_models{};
private:
	inline static Renderer* rendererHandle = nullptr;
};

