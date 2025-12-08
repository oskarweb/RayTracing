#pragma once

#include "extras.hpp"
#include "renderer.hpp"
#include "renderer_extras.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <map>

inline void printVec3(glm::vec3 &vec, const std::string &name = "")
{
    std::cout << name << vec.x << " " << vec.y << " " << vec.z << std::endl;
}

struct Model
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
        _renderables = std::map<std::string, std::multimap<std::string, Renderable, RenderableComp>::iterator>();
        _renderableInfos = std::vector<RenderableInfo>();
    }
    Model(glm::vec3 pos, glm::vec3 faceDirection) : _pos(pos), _faceDirection(faceDirection), _offset(glm::vec3(0.0f))
    {
    }
    Model(glm::vec3 pos, glm::vec3 faceDirection, glm::vec3 offset)
        : _pos(pos), _faceDirection(faceDirection), _offset(offset)
    {
    }
    glm::vec3 _pos;
    glm::vec3 _faceDirection;
    glm::vec3 _offset;
    std::map<std::string, std::multimap<std::string, Renderable, RenderableComp>::iterator> _renderables;
    std::vector<RenderableInfo> _renderableInfos;

    virtual void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                        glm::vec3 offset = glm::vec3(0.0f)) = 0;

    void cleanup(Renderer *rendererHandle)
    {
        for (auto &[name, renderableIt] : _renderables)
        {
            rendererHandle->removeRenderable(renderableIt);
        }
        _renderables.clear();
    }
};

struct VectorArrowModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        _pos = pos;
        _faceDirection = glm::normalize(faceDirection);
        _offset = offset;

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), _pos + _offset);

        glm::quat rotation = glm::rotation(Constants::WORLD_UP, faceDirection);
        (*_renderables["head"]).second.transformMatrix = translation * glm::toMat4(rotation);
    }

    VectorArrowModel(glm::vec3 pos, glm::vec3 faceDirection, glm::vec3 offset) : Model(pos, faceDirection, offset)
    {
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"head", "pyramid", "cube", glm::translate(glm::mat4(1.0f), _pos)}};
    }
};

struct ParticleModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["particleBody"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    ParticleModel(glm::vec3 pos) : Model(pos)
    {
        _renderableInfos = {RenderableInfo{"particleBody", "cube", "cube", glm::translate(glm::mat4(1.0f), _pos)}};
    }
};

struct RedLineModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["trail"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    RedLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"trail", "redline", "line", translation * glm::toMat4(rotation) * scale}};
    }
};

struct YellowLineModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["trail"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    YellowLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"trail", "yellowline", "line", translation * glm::toMat4(rotation) * scale}};
    }
};

struct GreenLineModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["trail"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    GreenLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"trail", "greenline", "line", translation * glm::toMat4(rotation) * scale}};
    }
};

struct AxesModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["xAxis"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        (*_renderables["yAxis"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
        (*_renderables["zAxis"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    AxesModel(glm::vec3 pos) : Model(pos)
    {
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"xAxis", "xAxis", "line", glm::translate(glm::mat4(1.0f), _pos)},
            RenderableInfo{"yAxis", "yAxis", "line", glm::translate(glm::mat4(1.0f), _pos)},
            RenderableInfo{"zAxis", "zAxis", "line", glm::translate(glm::mat4(1.0f), _pos)}};
    }
};

struct CuboidModel : Model
{
    using Model::_renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["cuboid"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    CuboidModel(glm::vec3 pos, glm::vec3 shape) : Model(pos), _shape(shape)
    {
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"cuboid", "cube", "cube",
                           glm::translate(glm::mat4(1.0f), _pos) * glm::scale(glm::mat4(1.0f), glm::vec3(_shape))}};
    }

    glm::vec3 _shape;
};

struct ParaboloidModel : Model
{
    void update(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*_renderables["paraboloid"]).second.transformMatrix = glm::translate(glm::mat4(1.0f), pos);
    }

    ParaboloidModel(glm::vec3 pos, glm::vec3 scale, const std::string &meshName) : Model(pos)
    {
        _renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"paraboloid", meshName, "paraboloid",
                           glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(scale))}};
    }
};