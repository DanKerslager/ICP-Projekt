#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "App.hpp"
#include "gl_err_callback.hpp"

namespace {
    constexpr int GL_REQUIRED_MAJOR = 4;
    constexpr int GL_REQUIRED_MINOR = 6;

    // glGetString() may return nullptr
    const char* gl_string(GLenum name)
    {
        const GLubyte* s = glGetString(name);
        return s ? reinterpret_cast<const char*>(s) : "<Unknown>";
    }
}

App::~App()
{
    if (window)
        glfwDestroyWindow(window);
    if (glfw_initialized)
        glfwTerminate();
    std::cout << "Bye!\n";
}

void App::init()
{
    load_config("resources/app_settings.json");
    init_glfw();
    init_glew();
    print_gl_info();
    init_gl_debug();
    set_vsync(vsync_on);
}

void App::load_config(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Config '" << path.generic_string() << "' not found, using defaults.\n";
        return;
    }

    nlohmann::json settings = nlohmann::json::parse(file); // throws on malformed JSON

    if (settings["appname"].is_string())
        app_name = settings["appname"].get<std::string>();
    if (settings["default_resolution"]["x"].is_number_integer())
        window_width = settings["default_resolution"]["x"].get<int>();
    if (settings["default_resolution"]["y"].is_number_integer())
        window_height = settings["default_resolution"]["y"].get<int>();
    if (settings["vsync"].is_boolean())
        vsync_on = settings["vsync"].get<bool>();

    if (window_width <= 0 || window_height <= 0)
        throw std::runtime_error(std::format("Invalid window size in config: {}x{}", window_width, window_height));
}

void App::init_glfw()
{
    // must be set before glfwInit() to catch init errors
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed.");
    glfw_initialized = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_REQUIRED_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_REQUIRED_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);

    window = glfwCreateWindow(window_width, window_height, app_name.c_str(), nullptr, nullptr);
    if (!window)
        throw std::runtime_error(std::format("Failed to create window with OpenGL {}.{} CORE context.",
            GL_REQUIRED_MAJOR, GL_REQUIRED_MINOR));

    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, fbsize_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

void App::init_glew()
{
    GLenum err = glewInit();
    if (err != GLEW_OK)
        throw std::runtime_error(std::string("GLEW init failed: ") +
            reinterpret_cast<const char*>(glewGetErrorString(err)));

    std::cout << "GLFW " << glfwGetVersionString() << '\n';
    std::cout << "GLEW " << glewGetString(GLEW_VERSION) << '\n';
}

void App::print_gl_info()
{
    std::cout << "GL vendor:   " << gl_string(GL_VENDOR) << '\n'
              << "GL renderer: " << gl_string(GL_RENDERER) << '\n'
              << "GL version:  " << gl_string(GL_VERSION) << '\n'
              << "GLSL:        " << gl_string(GL_SHADING_LANGUAGE_VERSION) << '\n';

    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "GL context:  " << major << '.' << minor;
    if (major < GL_REQUIRED_MAJOR || (major == GL_REQUIRED_MAJOR && minor < GL_REQUIRED_MINOR))
        throw std::runtime_error(std::format("OpenGL {}.{} required, got {}.{}",
            GL_REQUIRED_MAJOR, GL_REQUIRED_MINOR, major, minor));

    GLint profile = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    if (profile & GL_CONTEXT_CORE_PROFILE_BIT)
        std::cout << " CORE profile";
    else if (profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
        std::cout << " COMPATIBILITY profile";
    else
        throw std::runtime_error("Unknown GL context profile.");

    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    std::cout << "\nGL flags:   ";
    if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) std::cout << " FORWARD_COMPATIBLE";
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)              std::cout << " DEBUG";
    if (flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)      std::cout << " ROBUST_ACCESS";
    if (flags & GL_CONTEXT_FLAG_NO_ERROR_BIT)           std::cout << " NO_ERROR";
    if (flags == 0)                                     std::cout << " <none>";
    std::cout << '\n';
}

void App::init_gl_debug()
{
    if (!GLEW_ARB_debug_output) {
        std::cout << "GL_DEBUG NOT SUPPORTED!\n";
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // report on the offending call, like glGetError()
    glDebugMessageCallback(MessageCallback, nullptr);

    // suppress noisy notifications (buffer placement info etc.)
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    std::cout << "GL_DEBUG enabled.\n";
}

void App::set_vsync(bool on)
{
    vsync_on = on;
    glfwSwapInterval(vsync_on ? 1 : 0);
    update_title();
}

void App::update_title()
{
    std::string title = std::format("{} | FPS: {:.0f} | VSync: {}", app_name, fps, vsync_on ? "ON" : "OFF");
    glfwSetWindowTitle(window, title.c_str());
}

int App::run()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    double last_time = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // ... draw calls ...

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frames;
        double now = glfwGetTime();
        double elapsed = now - last_time;
        if (elapsed >= 1.0) {
            fps = frames / elapsed;
            frames = 0;
            last_time = now;
            update_title();
        }
    }
    return EXIT_SUCCESS;
}

//
// callbacks
//

void App::error_callback(int error, const char* description)
{
    // Do not throw here: exceptions must not unwind through GLFW's C code.
    // Failures are detected by return values (glfwInit, glfwCreateWindow...) and thrown there.
    std::cerr << "GLFW error " << error << ": " << description << '\n';
}

void App::key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS)
        return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_V: {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        app->set_vsync(!app->vsync_on);
        std::cout << "VSync " << (app->vsync_on ? "ON" : "OFF") << '\n';
        break;
    }
    default:
        break;
    }
}

void App::fbsize_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (action != GLFW_PRESS)
        return;

    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    std::cout << "Mouse button " << button << " click at [" << app->cursor_x << ", " << app->cursor_y << "]\n";
}

void App::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    // only store - printing every move would flood the console
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->cursor_x = xpos;
    app->cursor_y = ypos;
}

void App::scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    if (yoffset > 0.0)
        std::cout << "Wheel up\n";
    else if (yoffset < 0.0)
        std::cout << "Wheel down\n";
}
