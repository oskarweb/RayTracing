#pragma once

#include "core/extras.hpp"
#include "core/scene/base_object.hpp"
#include "core/scene/renderer_extras.hpp"
#include "core/sparse_set.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cmath>
#include <stdexcept>
#include <unordered_map>

inline glm::vec3 safeDirection(glm::vec3 direction, glm::vec3 fallback = glm::vec3(0.0f))
{
    const float length = glm::length(direction);
    return std::isfinite(length) && length > 0.0f ? direction / length : fallback;
}

struct Model : public BaseObject
{
    Model() = delete;
    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;
    Model(Model &&) = delete;
    Model &operator=(Model &&) = delete;

    virtual ~Model() { cleanup(); }

    Model(glm::vec3 pos)
    {
        _pos = pos;
        _faceDirection = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
        _offset = glm::vec3(0.0f);
    }
    Model(glm::vec3 pos, glm::vec3 faceDirection) : _pos(pos), _faceDirection(faceDirection), _offset(glm::vec3(0.0f))
    {
    }
    Model(glm::vec3 pos, glm::vec3 faceDirection, glm::vec3 offset)
        : _pos(pos), _faceDirection(faceDirection), _offset(offset)
    {
    }

    virtual void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                        glm::vec3 offset = glm::vec3(0.0f)) = 0;

    void cleanup()
    {
        for (auto &[name, objectHandle] : _renderObjectHandles)
        {
            if (!m_rendererLifetime.expired())
                m_owner->removeRenderObject(objectHandle);
        }
        _renderObjectHandles.clear();
    }
    RenderObject *renderObject(const std::string &name)
    {
        if (m_rendererLifetime.expired())
            throw std::logic_error("Model renderer has been cleaned up");
        auto object = m_owner->getRenderObject(_renderObjectHandles.at(name));
        if (!object)
            throw std::logic_error("Model render object has been removed");
        return object;
    }
    Renderer *m_owner = requireRenderer();
    std::weak_ptr<void> m_rendererLifetime = m_owner ? m_owner->modelLifetime() : std::weak_ptr<void>{};
    glm::vec3 _pos;
    glm::vec3 _faceDirection;
    glm::vec3 _offset;
    std::unordered_map<std::string, SparseSet<RenderObject>::Handle> _renderObjectHandles{};
};

struct VectorArrowModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        _pos = pos;
        _faceDirection = safeDirection(faceDirection, Constants::WORLD_UP);
        _offset = offset;

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), _pos + _offset);

        glm::quat rotation = glm::rotation(Constants::WORLD_UP, _faceDirection);
        auto head = renderObject("head");
        head->transformMatrix = translation * glm::toMat4(rotation);
    }

    VectorArrowModel(glm::vec3 pos, glm::vec3 faceDirection, glm::vec3 offset) : Model(pos, faceDirection, offset)
    {
        auto rHandle = m_owner;
        _renderObjectHandles["head"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("pyramid"), rHandle->getMaterial("cube"), glm::translate(glm::mat4(1.0f), _pos)});
    }
};

struct ParticleModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto particleBody = renderObject("particleBody");
        particleBody->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    ParticleModel(glm::vec3 pos) : Model(pos)
    {
        auto rHandle = m_owner;
        _renderObjectHandles["particleBody"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("cube"), rHandle->getMaterial("cube"), glm::translate(glm::mat4(1.0f), _pos)});
    }
};

struct RedLineModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        _pos = pos;
        _faceDirection = safeDirection(faceDirection, glm::vec3(1.0f, 0.0f, 0.0f));
        _offset = offset;

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), _pos);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), _offset);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), _faceDirection);
        auto head = renderObject("trail");
        head->transformMatrix = translation * glm::toMat4(rotation) * scale;
    }

    RedLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = safeDirection(dir, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        auto rHandle = m_owner;
        _renderObjectHandles["trail"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("redline"), rHandle->getMaterial("line"), translation * glm::toMat4(rotation) * scale});
    }
};

struct YellowLineModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto trail = renderObject("trail");
        trail->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    YellowLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = safeDirection(dir, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        auto rHandle = m_owner;
        _renderObjectHandles["trail"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("yellowline"), rHandle->getMaterial("line"), translation * glm::toMat4(rotation) * scale});
    }
};

struct GreenLineModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto trail = renderObject("trail");
        trail->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    GreenLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = safeDirection(dir, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        auto rHandle = m_owner;
        _renderObjectHandles["trail"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("greenline"), rHandle->getMaterial("line"), translation * glm::toMat4(rotation) * scale});
    }
};

struct AxesModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto xAxis = renderObject("xAxis");
        auto yAxis = renderObject("yAxis");
        auto zAxis = renderObject("zAxis");
        xAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        yAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        zAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    AxesModel(glm::vec3 pos) : Model(pos)
    {
        auto rHandle = m_owner;
        _renderObjectHandles["xAxis"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("xAxis"), rHandle->getMaterial("line"), glm::translate(glm::mat4(1.0f), _pos)});
        _renderObjectHandles["yAxis"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("yAxis"), rHandle->getMaterial("line"), glm::translate(glm::mat4(1.0f), _pos)});
        _renderObjectHandles["zAxis"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("zAxis"), rHandle->getMaterial("line"), glm::translate(glm::mat4(1.0f), _pos)});
    }
};

struct CuboidModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto cuboid = renderObject("cuboid");
        cuboid->transformMatrix = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), _shape);
    }

    CuboidModel(glm::vec3 pos, glm::vec3 shape) : Model(pos), _shape(shape)
    {
        auto rHandle = m_owner;
        _renderObjectHandles["cuboid"] = rHandle->addRenderObject(
            RenderObject{rHandle->getMesh("cube"), rHandle->getMaterial("cube"),
                         glm::translate(glm::mat4(1.0f), _pos) * glm::scale(glm::mat4(1.0f), glm::vec3(_shape))});
    }

    glm::vec3 _shape;
};

struct ParaboloidModel : Model
{
    glm::vec3 _scale;
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto paraboloid = renderObject("paraboloid");
        paraboloid->transformMatrix = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), _scale);
    }

    ParaboloidModel(glm::vec3 pos, glm::vec3 scale, const std::string &meshName) : Model(pos), _scale(scale)
    {
        auto rHandle = m_owner;
        _renderObjectHandles["paraboloid"] = rHandle->addRenderObject(
            RenderObject{rHandle->getMesh(meshName), rHandle->getMaterial("paraboloid"),
                         glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(scale))});
    }
};
