#include "types.h"
#if RENDERER_BACKEND == OpenGL
#include "glfw_gl.h"
#else
#include "glfw_vk.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/renderer_system.h"
#include "world.h"
#include "renderer/test_feature.h"
#include "window.h"

int main() {
    glfwSetErrorCallback([](int error, const char *description) { fprintf(stderr, "Error: %s\n", description); });

    const u32   window_width = 1280;
    const u32   window_height = 720;
    const char *window_title = "game";

    if (!glfwInit())
        exit(EXIT_FAILURE);

#if RENDERER_BACKEND == OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

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


#if RENDERER_BACKEND == OpenGL
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);
#endif

    auto &rendererSystem = xjar::RendererSystem::Instance();

    rendererSystem.Startup();
    rendererSystem.SetView(glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f, 0.0f, -1.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f)));


    auto &world = xjar::World::Instance();
    xjar::Entity *triangle = world.CreateEntity();
    triangle->model.vertices = {
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
    
    triangle->model.localTransform = glm::rotate(glm::mat4(1.0f), 45.0f, glm::vec3(0.0f, 0.0f, 1.0f));

    f32 frameTime = static_cast<f32>(glfwGetTime());

    rendererSystem.LoadModel(triangle->model);

    while (!glfwWindowShouldClose(window)) {
        f32 currentTime = glfwGetTime();
        f32 dtForFrame = currentTime - static_cast<f32>(frameTime);

        frameTime = currentTime;

        glfwPollEvents();

        if (auto *cmdbuf = rendererSystem.BeginFrame()) {
            rendererSystem.BeginDefaultPass();
           
            rendererSystem.testFeature->DrawEntities(cmdbuf, {triangle});

            rendererSystem.EndDefaultPass();

            rendererSystem.EndFrame();
        }
    }


    rendererSystem.Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
