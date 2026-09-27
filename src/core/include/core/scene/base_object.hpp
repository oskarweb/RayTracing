#pragma once

#include "core/scene/renderer.hpp"
#include <stdexcept>

class BaseObject
{
public:
    inline static void setRenderer(Renderer *renderer)
    {
        rendererHandle = renderer;
        rendererLifetime = renderer ? renderer->objectLifetime() : std::weak_ptr<void>{};
    }

protected:
    static Renderer *requireRenderer()
    {
        if (rendererLifetime.expired() || rendererHandle->modelLifetime().expired())
            throw std::logic_error("No active renderer for model");
        return rendererHandle;
    }
    inline static std::weak_ptr<void> rendererLifetime;
    inline static Renderer *rendererHandle = nullptr;
};
