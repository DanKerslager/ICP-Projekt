#pragma once

#include <filesystem>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class App {
public:
    App() = default;
    ~App();

    void init();   // throws std::runtime_error on failure
    int run();

private:
    // config (overridden by resources/app_settings.json)
    std::string app_name = "ICP";
    int window_width = 800;
    int window_height = 600;
    bool vsync_on = true;

    // runtime state
    GLFWwindow* window = nullptr;
    bool glfw_initialized = false;
    double fps = 0.0;
    double cursor_x = 0.0;
    double cursor_y = 0.0;

    void load_config(const std::filesystem::path& path);
    void init_glfw();
    void init_glew();
    void init_gl_debug();
    void print_gl_info();

    void set_vsync(bool on);
    void update_title();

    // GLFW callbacks (static: GLFW is a C API; App* is fetched via glfwGetWindowUserPointer)
    static void error_callback(int error, const char* description);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void fbsize_callback(GLFWwindow* window, int width, int height);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};
