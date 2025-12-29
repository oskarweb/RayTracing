#pragma once

#include "renderer.hpp"

class BaseObject
{
public:
    inline static void setRenderer(Renderer *renderer) { rendererHandle = renderer; }

protected:
    inline static Renderer *rendererHandle = nullptr;
};