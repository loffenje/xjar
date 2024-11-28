#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/renderer_system.h"
#include "window.h"
#include "types.h"

int main() {
    glfwSetErrorCallback([](int error, const char *description) { fprintf(stderr, "Error: %s\n", description); });

    const u32 window_width = 1280;
    const u32 window_height = 720;
    const char *window_title = "game";

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(window_width, window_height, window_title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    xjar::SetWindowParams(window_width, window_height, window_title, window, glfwGetWin32Window(window));

    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int w, int h) {
        xjar::RendererSystem::Instance().OnResized(w, h);
    });

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    auto &rendererSystem = xjar::RendererSystem::Instance();

    rendererSystem.Startup(xjar::RendererBackendType::OpenGL);
    rendererSystem.SetView(glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f, 0.0f, -1.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f)));

    f32 frameTime = static_cast<f32>(glfwGetTime());
    while (!glfwWindowShouldClose(window)) {
        f32 currentTime = glfwGetTime();
        f32 dtForFrame = currentTime - static_cast<f32>(frameTime);

        frameTime = currentTime;

        glfwPollEvents();

        rendererSystem.DrawFrame(dtForFrame);
    }

    rendererSystem.Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
