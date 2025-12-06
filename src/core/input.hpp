#pragma once

#include <GLFW/glfw3.h>

#include <map>
#include <utility>

class Input
{
  public:
    struct Key
    {
        bool pressed = false;
        bool justPressed = false;
    };

    struct Point
    {
        double x = 0.0F;
        double y = 0.0F;
    };

    inline static std::map<int, Key> keyStates = {
        {GLFW_KEY_W, Key{false, false}}, {GLFW_KEY_S, Key{false, false}},
        {GLFW_KEY_A, Key{false, false}}, {GLFW_KEY_D, Key{false, false}},
        {GLFW_KEY_C, Key{false, false}}, {GLFW_KEY_F, Key{false, false}}};

    inline static void setWindow(GLFWwindow *window)
    {
        windowHandle = window;
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
    }

    inline static const Point &getMousePos() { return mousePos; }

    inline static bool isPressed(int key) { return keyStates.at(key).pressed; }

    inline static bool isJustPressed(int key)
    {
        if (keyStates.at(key).justPressed)
        {
            keyStates.at(key).justPressed = false;
            return true;
        }
        return false;
    }

    inline static void mouseCallback(GLFWwindow *window, double xpos,
                                     double ypos);
    inline static void keyCallback(GLFWwindow *window, int key, int scancode,
                                   int action, int mods);

    inline static Point mousePos = {0.0, 0.0};
    inline static GLFWwindow *windowHandle = nullptr;
};

void Input::mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    Input::mousePos = {xpos, ypos};
}

void Input::keyCallback(GLFWwindow *window, int key, int scancode, int action,
                        int mods)
{
    if (action == GLFW_PRESS && Input::keyStates.contains(key) &&
        !Input::isPressed(key))
    {
        Input::keyStates[key].pressed = true;
    }
    if (action == GLFW_RELEASE && Input::keyStates.contains(key) &&
        Input::isPressed(key))
    {
        Input::keyStates[key].pressed = false;
        Input::keyStates[key].justPressed = true;
    }
}