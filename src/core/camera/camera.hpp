#pragma once

#include "core/camera/input.hpp"
#include "core/extras.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera
{
public:
    Camera() : m_position(glm::vec3(0.0f, 0.0f, 0.0f)), m_pitch(0.0f), m_yaw(0.0f) {}

    Camera(glm::vec3 position, float pitch, float yaw, glm::vec3 up) : m_position(position), m_pitch(pitch), m_yaw(yaw)
    {
    }

    [[nodiscard]] glm::mat4 getViewMatrix() const
    {
        if (m_lookingAtOrigin && glm::dot(m_position, m_position) > 0.0f)
        {
            const auto direction = glm::normalize(-m_position);
            const auto up = std::abs(glm::dot(direction, Constants::WORLD_UP)) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                                                                        : -Constants::WORLD_UP;
            return glm::lookAt(m_position, glm::vec3(0.0f), up);
        }
        glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), m_position);
        glm::mat4 cameraRotation = getRotationMatrix();
        return glm::inverse(cameraTranslation * cameraRotation);
    }

    [[nodiscard]] glm::mat4 getRotationMatrix() const
    {
        glm::quat pitchRotation = glm::angleAxis(m_pitch, glm::vec3{1.f, 0.f, 0.f});
        glm::quat yawRotation = glm::angleAxis(m_yaw, glm::vec3{0.f, 1.f, 0.f});

        return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
    }

    void processKeyboardInput()
    {
        if (Input::isPressedKeyboard(GLFW_KEY_W) && !Input::isPressedKeyboard(GLFW_KEY_S))
        {
            m_velocity.z = m_maxVelocity;
        }
        else if (Input::isPressedKeyboard(GLFW_KEY_S) && !Input::isPressedKeyboard(GLFW_KEY_W))
        {
            m_velocity.z = -m_maxVelocity;
        }
        else
        {
            m_velocity.z = 0.0f;
        }

        if (Input::isPressedKeyboard(GLFW_KEY_A) && !Input::isPressedKeyboard(GLFW_KEY_D))
        {
            m_velocity.x = -m_maxVelocity;
        }
        else if (Input::isPressedKeyboard(GLFW_KEY_D) && !Input::isPressedKeyboard(GLFW_KEY_A))
        {
            m_velocity.x = m_maxVelocity;
        }
        else
        {
            m_velocity.x = 0.0f;
        }

        if (Input::isJustPressedKeyboard(GLFW_KEY_C))
        {
            m_locked = !m_locked;
        }
        if (Input::isJustPressedKeyboard(GLFW_KEY_F))
        {
            m_lookingAtOrigin = !m_lookingAtOrigin;
        }
    }

    void processMouseInput()
    {
        double xoffset = Input::getMousePos().x - m_lastX;
        double yoffset = m_lastY - Input::getMousePos().y;

        m_lastX = Input::getMousePos().x;
        m_lastY = Input::getMousePos().y;

        if (m_locked)
        {
            return;
        }

        xoffset *= m_sensitivity;
        yoffset *= m_sensitivity;

        m_yaw += static_cast<float>(xoffset);
        m_pitch += static_cast<float>(yoffset);

        if (m_lookingAtOrigin)
        {
            m_position = glm::rotate(glm::mat4(1.0f), static_cast<float>(xoffset), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::vec4(m_position, 1.0f);
            const glm::vec3 axis = glm::cross(m_position, glm::vec3(0.0f, 1.0f, 0.0f));
            if (glm::dot(axis, axis) > 0.0f && m_pitch > glm::radians(-ORIGIN_CAMERA_PITCH_LIMIT) &&
                m_pitch < glm::radians(ORIGIN_CAMERA_PITCH_LIMIT))
            {
                m_position = glm::rotate(glm::mat4(1.0f), static_cast<float>(yoffset), glm::normalize(axis)) *
                             glm::vec4(m_position, 1.0f);
            }
            m_pitch =
                std::clamp(m_pitch, glm::radians(-ORIGIN_CAMERA_PITCH_LIMIT), glm::radians(ORIGIN_CAMERA_PITCH_LIMIT));
            return;
        }

        m_pitch = std::clamp(m_pitch, glm::radians(-89.0f), glm::radians(89.0f));
    }

    void update(float deltaTime)
    {
        processKeyboardInput();
        processMouseInput();
        glm::mat4 cameraRotation = getRotationMatrix();
        m_position += glm::vec3(cameraRotation * glm::vec4(m_velocity * deltaTime, 0.f));
    }

    void calculatePitchYaw()
    {
        const auto displacement = m_target - m_position;
        if (glm::dot(displacement, displacement) == 0.0f)
            return;
        const auto direction = glm::normalize(displacement);
        m_yaw = std::atan2(direction.x, direction.z);
        m_pitch = std::asin(std::clamp(-direction.y, -1.0f, 1.0f));
    }

    glm::vec3 &velocity() { return m_velocity; }
    glm::vec3 &position() { return m_position; }
    void setVelocityX(float velocityX) noexcept { m_velocity.x = velocityX; }
    void setVelocityY(float velocityY) noexcept { m_velocity.y = velocityY; }
    void setVelocityZ(float velocityZ) noexcept { m_velocity.z = velocityZ; }
    void setPitch(float pitch) { m_pitch = pitch; }
    void setYaw(float yaw) { m_yaw = yaw; }
    bool locked() const { return m_locked; }
    void lock(bool locked) { m_locked = locked; }
    [[nodiscard]] float getMaxVelocity() const noexcept { return m_maxVelocity; }

private:
    bool m_locked = true;
    bool m_lookingAtOrigin = true;
    glm::vec3 m_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_position;
    glm::vec3 m_target = glm::vec3(0.0f, 0.0f, 0.0f);
    float m_pitch{0.f};
    float m_yaw{0.f};
    const float m_maxVelocity{20.0f};
    double m_lastX{0.0};
    double m_lastY{0.0};
    float m_sensitivity = 0.01f;

    float ORIGIN_CAMERA_PITCH_LIMIT = 45.0f;
};
