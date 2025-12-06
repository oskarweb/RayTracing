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

    Model(glm::vec3 _pos)
    {
        pos = _pos;
        faceDirection = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
        offset = glm::vec3(0.0f);
        renderables =
            std::map<std::string, std::multimap<std::string, Renderable,
                                                RenderableComp>::iterator>();
        renderableInfos = std::vector<RenderableInfo>();
    }
    Model(glm::vec3 _pos, glm::vec3 _faceDirection)
        : pos(_pos), faceDirection(_faceDirection), offset(glm::vec3(0.0f))
    {
    }
    Model(glm::vec3 _pos, glm::vec3 _faceDirection, glm::vec3 _offset)
        : pos(_pos), faceDirection(_faceDirection), offset(_offset)
    {
    }
    glm::vec3 pos;
    glm::vec3 faceDirection;
    glm::vec3 offset;
    std::map<std::string,
             std::multimap<std::string, Renderable, RenderableComp>::iterator>
        renderables;
    std::vector<RenderableInfo> renderableInfos;

    virtual void update(glm::vec3 pos = glm::vec3(0.0f),
                        glm::vec3 faceDirection = glm::vec3(0.0f),
                        glm::vec3 offset = glm::vec3(0.0f)) = 0;

    void cleanup(Renderer *rendererHandle)
    {
        for (auto &[name, renderableIt] : renderables)
        {
            rendererHandle->removeRenderable(renderableIt);
        }
        renderables.clear();
    }
};

struct VectorArrowModel : Model
{
    using Model::renderables;

    void update(glm::vec3 _pos = glm::vec3(0.0f),
                glm::vec3 _faceDirection = glm::vec3(0.0f),
                glm::vec3 _offset = glm::vec3(0.0f)) override
    {
        pos = _pos;
        faceDirection = glm::normalize(_faceDirection);
        offset = _offset;

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos + offset);

        glm::quat rotation = glm::rotation(Constants::WORLD_UP, faceDirection);
        (*renderables["head"]).second.transformMatrix =
            translation * glm::toMat4(rotation);
    }

    VectorArrowModel(glm::vec3 _pos, glm::vec3 _faceDirection,
                     glm::vec3 _offset)
        : Model(_pos, _faceDirection, _offset)
    {
        renderableInfos = std::vector<RenderableInfo>{RenderableInfo{
            "head", "pyramid", "cube", glm::translate(glm::mat4(1.0f), pos)}};
    }
};

struct ParticleModel : Model
{
    using Model::renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*renderables["particleBody"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
    }

    ParticleModel(glm::vec3 _pos) : Model(_pos)
    {
        renderableInfos = {
            RenderableInfo{"particleBody", "cube", "cube",
                           glm::translate(glm::mat4(1.0f), pos)}};
    }
};

struct RedLineModel : Model
{
    using Model::renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*renderables["trail"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
    }

    RedLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation =
            glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale =
            glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"trail", "redline", "line",
                           translation * glm::toMat4(rotation) * scale}};
    }
};

struct YellowLineModel : Model
{
    using Model::renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*renderables["trail"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
    }

    YellowLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation =
            glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale =
            glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"trail", "yellowline", "line",
                           translation * glm::toMat4(rotation) * scale}};
    }
};

struct GreenLineModel : Model
{
    using Model::renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*renderables["trail"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
    }

    GreenLineModel(glm::vec3 from, glm::vec3 to) : Model(from)
    {
        glm::vec3 dir = to - from;
        float length = glm::length(dir);
        glm::vec3 normDir = glm::normalize(dir);
        glm::quat rotation =
            glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), normDir);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), from);
        glm::mat4 scale =
            glm::scale(glm::mat4(1.0f), glm::vec3(length, 1.0f, 1.0f));
        renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"trail", "greenline", "line",
                           translation * glm::toMat4(rotation) * scale}};
    }
};

struct AxesModel : Model
{
    using Model::renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*renderables["xAxis"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
        (*renderables["yAxis"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
        (*renderables["zAxis"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
    }

    AxesModel(glm::vec3 _pos) : Model(_pos)
    {
        renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"xAxis", "xAxis", "line",
                           glm::translate(glm::mat4(1.0f), pos)},
            RenderableInfo{"yAxis", "yAxis", "line",
                           glm::translate(glm::mat4(1.0f), pos)},
            RenderableInfo{"zAxis", "zAxis", "line",
                           glm::translate(glm::mat4(1.0f), pos)}};
    }
};

struct CuboidModel : Model
{
    using Model::renderables;

    void update(glm::vec3 pos = glm::vec3(0.0f),
                glm::vec3 faceDirection = glm::vec3(0.0f),
                glm::vec3 offset = glm::vec3(0.0f)) override
    {
        (*renderables["cuboid"]).second.transformMatrix =
            glm::translate(glm::mat4(1.0f), pos);
    }

    CuboidModel(glm::vec3 _pos, glm::vec3 _shape) : Model(_pos), shape(_shape)
    {
        renderableInfos = std::vector<RenderableInfo>{
            RenderableInfo{"cuboid", "cube", "cube",
                           glm::translate(glm::mat4(1.0f), pos) *
                               glm::scale(glm::mat4(1.0f), glm::vec3(shape))}};
    }

    glm::vec3 shape;
};