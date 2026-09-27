#pragma once

#include <GLFW/glfw3.h>

#include <unordered_map>
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

    inline static std::unordered_map<int, Key> mouseButtonStates = {{GLFW_MOUSE_BUTTON_LEFT, Key{false, false}},
                                                                    {GLFW_MOUSE_BUTTON_RIGHT, Key{false, false}}};

    inline static std::unordered_map<int, Key> keyStates = {
        {GLFW_KEY_W, Key{false, false}}, {GLFW_KEY_S, Key{false, false}}, {GLFW_KEY_A, Key{false, false}},
        {GLFW_KEY_D, Key{false, false}}, {GLFW_KEY_C, Key{false, false}}, {GLFW_KEY_F, Key{false, false}}};

    inline static void setWindow(GLFWwindow *window)
    {
        windowHandle = window;
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
    }

    inline static const Point &getMousePos() { return mousePos; }

    inline static bool isPressedKeyboard(int key)
    {
        auto it = keyStates.find(key);
        return it != keyStates.end() && it->second.pressed;
    }

    inline static bool isJustPressedKeyboard(int key)
    {
        auto it = keyStates.find(key);
        if (it != keyStates.end() && it->second.justPressed)
        {
            it->second.justPressed = false;
            return true;
        }
        return false;
    }

    inline static bool isPressedMouse(int key)
    {
        auto it = mouseButtonStates.find(key);
        return it != mouseButtonStates.end() && it->second.pressed;
    }

    inline static bool isJustPressedMouse(int key)
    {
        auto it = mouseButtonStates.find(key);
        if (it != mouseButtonStates.end() && it->second.justPressed)
        {
            it->second.justPressed = false;
            return true;
        }
        return false;
    }

    inline static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    inline static void mouseCallback(GLFWwindow *window, double xpos, double ypos);
    inline static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    inline static Point mousePos = {0.0, 0.0};
    inline static GLFWwindow *windowHandle = nullptr;
};

void Input::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS && Input::mouseButtonStates.contains(button) && !Input::isPressedMouse(button))
    {
        Input::mouseButtonStates[button].pressed = true;
        Input::mouseButtonStates[button].justPressed = true;
    }
    if (action == GLFW_RELEASE && Input::mouseButtonStates.contains(button) && Input::isPressedMouse(button))
    {
        Input::mouseButtonStates[button].pressed = false;
    }
}

void Input::mouseCallback(GLFWwindow *window, double xpos, double ypos) { Input::mousePos = {xpos, ypos}; }

void Input::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && Input::keyStates.contains(key) && !Input::isPressedKeyboard(key))
    {
        Input::keyStates[key].pressed = true;
        keyStates[key].justPressed = true;
    }
    if (action == GLFW_RELEASE && Input::keyStates.contains(key) && Input::isPressedKeyboard(key))
    {
        Input::keyStates[key].pressed = false;
    }
}