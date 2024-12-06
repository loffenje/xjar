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



    auto &world = xjar::World::Instance();
    xjar::Entity *triangle = world.CreateEntity();
    triangle->model.vertices = {
       // left face (white)
    {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
    {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
    {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
    {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
    {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
    {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},

    // right face (yellow)
    {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
    {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
    {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
    {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
    {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
    {{.5f, .5f, .5f}, {.8f, .8f, .1f}},

    // top face (orange, remember y axis points down)
        {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
    {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
    {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
    {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
    {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
    {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},

    // bottom face (red)
        {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
    {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
    {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
    {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
    {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
    {{.5f, .5f, .5f}, {.8f, .1f, .1f}},

    // nose face (blue)
        {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
    {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
    {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
    {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
    {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
    {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},

    // tail face (green)
     {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
    {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
    {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
    {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
};
;
    

    glm::vec3 camPos = {0.f, 0.f, -2.f};
    
    xjar::Camera camera;
    camera.view = glm::translate(glm::mat4(1.f), camPos);

    f32 frameTime = static_cast<f32>(glfwGetTime());

    auto windowObj = xjar::GetWindow();
    
    rendererSystem.LoadModel(triangle->model);

    while (!glfwWindowShouldClose(window)) {
    
        camera.projection = glm::perspective(glm::radians(45.0f), static_cast<f32>(windowObj.width) / static_cast<f32>(windowObj.height), 0.1f, 100.0f);
        camera.projection[1][1] *= -1;

        glm::mat4 localTransform = glm::rotate(glm::mat4(1.0f), (f32)glfwGetTime(), glm::vec3(1.0f, 0.0f, 1.0f));
        triangle->model.localTransform = localTransform;

        f32 currentTime = glfwGetTime();
        f32 dtForFrame = currentTime - static_cast<f32>(frameTime);

        frameTime = currentTime;

        glfwPollEvents();

        if (auto *cmdbuf = rendererSystem.BeginFrame()) {
            rendererSystem.BeginDefaultPass();
           
            rendererSystem.testFeature->DrawEntities(cmdbuf, camera, {triangle});

            rendererSystem.EndDefaultPass();

            rendererSystem.EndFrame();
        }
    }


    rendererSystem.Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
