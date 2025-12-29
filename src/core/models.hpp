#pragma once

#include "base_object.hpp"
#include "extras.hpp"
#include "renderer_extras.hpp"
#include "sparse_set.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <map>

inline void printVec3(glm::vec3 &vec, const std::string &name = "")
{
    std::cout << name << vec.x << " " << vec.y << " " << vec.z << std::endl;
}

struct Model : public BaseObject
{
    Model() = delete;
    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;
    Model(Model &&) = default;
    Model &operator=(Model &&) = default;

    Model(glm::vec3 pos)
    {
        _pos = _pos;
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
            BaseObject::rendererHandle->removeRenderObject(objectHandle);
        }
        _renderObjectHandles.clear();
    }
    glm::vec3 _pos;
    glm::vec3 _faceDirection;
    glm::vec3 _offset;
    std::map<std::string, SparseSet<RenderObject>::Handle> _renderObjectHandles{};
};

struct VectorArrowModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        _pos = pos;
        _faceDirection = glm::normalize(faceDirection);
        _offset = offset;

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), _pos + _offset);

        glm::quat rotation = glm::rotation(Constants::WORLD_UP, faceDirection);
        auto head = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["head"]);
        head->transformMatrix = translation * glm::toMat4(rotation);
    }

    VectorArrowModel(glm::vec3 pos, glm::vec3 faceDirection, glm::vec3 offset) : Model(pos, faceDirection, offset)
    {
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["head"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("pyramid"), rHandle->getMaterial("cube"), glm::translate(glm::mat4(1.0f), _pos)});
    }
};

struct ParticleModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto particleBody = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["particleBody"]);
        particleBody->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    ParticleModel(glm::vec3 pos) : Model(pos)
    {
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["particleBody"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("cube"), rHandle->getMaterial("cube"), glm::translate(glm::mat4(1.0f), _pos)});
    }
};

struct RedLineModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto trail = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["trail"]);
        trail->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    RedLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["trail"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("redline"), rHandle->getMaterial("line"), translation * glm::toMat4(rotation) * scale});
    }
};

struct YellowLineModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto trail = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["trail"]);
        trail->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    YellowLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["trail"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("yellowline"), rHandle->getMaterial("line"), translation * glm::toMat4(rotation) * scale});
    }
};

struct GreenLineModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto trail = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["trail"]);
        trail->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    GreenLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["trail"] = rHandle->addRenderObject(RenderObject{
            rHandle->getMesh("greenline"), rHandle->getMaterial("line"), translation * glm::toMat4(rotation) * scale});
    }
};

struct AxesModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto xAxis = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["xAxis"]);
        xAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        auto yAxis = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["xAxis"]);
        yAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        auto zAxis = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["xAxis"]);
        zAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        xAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        yAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        zAxis->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    AxesModel(glm::vec3 pos) : Model(pos)
    {
        auto rHandle = BaseObject::rendererHandle;
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
        auto cuboid = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["cuboid"]);
        cuboid->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    CuboidModel(glm::vec3 pos, glm::vec3 shape) : Model(pos), _shape(shape)
    {
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["cuboid"] = rHandle->addRenderObject(
            RenderObject{rHandle->getMesh("cube"), rHandle->getMaterial("cube"),
                         glm::translate(glm::mat4(1.0f), _pos) * glm::scale(glm::mat4(1.0f), glm::vec3(_shape))});
    }

    glm::vec3 _shape;
};

struct ParaboloidModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        auto paraboloid = BaseObject::rendererHandle->getRenderObject(_renderObjectHandles["paraboloid"]);
        paraboloid->transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    ParaboloidModel(glm::vec3 pos, glm::vec3 scale, const std::string &meshName) : Model(pos)
    {
        auto rHandle = BaseObject::rendererHandle;
        _renderObjectHandles["paraboloid"] = rHandle->addRenderObject(
            RenderObject{rHandle->getMesh(meshName), rHandle->getMaterial("paraboloid"),
                         glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(scale))});
    }
};