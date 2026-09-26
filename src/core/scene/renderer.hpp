#pragma once

#include "core/scene/renderer_extras.hpp"
#include "core/sparse_set.hpp"

#include <map>
#include <memory>
#include <string>

struct Model;

class Renderer
{
private:
public:
    Renderer() = default;
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;
    virtual ~Renderer() = default;
    std::weak_ptr<void> modelLifetime() const { return m_modelLifetime; }
    std::weak_ptr<void> objectLifetime() const { return m_objectLifetime; }
    virtual SparseSet<RenderObject>::Handle addRenderObject(RenderObject obj) = 0;
    virtual void removeRenderObject(SparseSet<RenderObject>::Handle handle) = 0;
    virtual SparseSet<Material>::Handle getMaterial(const std::string &name) = 0;
    virtual SparseSet<Mesh>::Handle getMesh(const std::string &name) = 0;
    virtual RenderObject *getRenderObject(SparseSet<RenderObject>::Handle handle) = 0;

protected:
    void invalidateModels() { m_modelLifetime.reset(); }
    void renewModelLifetime() { m_modelLifetime = std::make_shared<int>(0); }

private:
    std::shared_ptr<void> m_objectLifetime = std::make_shared<int>(0);
    std::shared_ptr<void> m_modelLifetime = std::make_shared<int>(0);
};
