#pragma once

#include "renderer_extras.hpp"

#include <map>
#include <string>

struct Model;

class Renderer
{
  private:
  public:
    virtual std::multimap<std::string, Renderable, RenderableComp>::iterator addRenderable(Renderable) = 0;
    virtual void addRenderables(Model *) = 0;
    virtual void removeRenderable(std::multimap<std::string, Renderable, RenderableComp>::iterator &) = 0;
};