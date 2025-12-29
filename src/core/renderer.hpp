#pragma once

#include "renderer_extras.hpp"
#include "sparse_set.hpp"

#include <map>
#include <string>

struct Model;

class Renderer
{
private:
public:
    virtual SparseSet<RenderObject>::Handle addRenderObject(RenderObject obj) = 0;
    virtual void removeRenderObject(SparseSet<RenderObject>::Handle handle) = 0;
    virtual SparseSet<Material>::Handle getMaterial(const std::string &name) = 0;
    virtual SparseSet<Mesh>::Handle getMesh(const std::string &name) = 0;
    virtual RenderObject *getRenderObject(SparseSet<RenderObject>::Handle handle) = 0;
};