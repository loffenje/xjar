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
#include "renderer/render_system.h"
#include "world.h"
#include "game_input.h"
#include "renderer/camera.h"
#include "tools/mesh_converter.h"
#include "renderer/mesh_feature.h"
#include "texture_manager.h"
#include "window.h"

static xjar::GameInput g_gameInput[2];
static xjar::GameInput *g_currInput;
static xjar::GameInput *g_prevInput;

static xjar::FirstPerson_Camera g_FpsCamera;

static void ProcessButton(xjar::ButtonState &button, b32 pressed) {
    if (button.pressed != pressed) {
        button.pressed = pressed;
        button.transitions++;
    }
}

glm::vec2 g_lastMousePos = glm::vec2(0.0f);

int main() {
    glfwSetErrorCallback([](int error, const char *description) { fprintf(stderr, "Error: %s\n", description); });

#if 1
    MeshConvert("assets/backpack/backpack.obj",
        "assets/test.mesh",
        "assets/test.mesh.instance",
        "assets/test.materials",
        "assets/backpack",
        true, true);
    #endif

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

    glfwSetCursorPosCallback(window, [](GLFWwindow *window, f64 xpos, f64 ypos) {
        
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        g_currInput->mouseX = static_cast<f32>(xpos / w);
        g_currInput->mouseY = static_cast<f32>(ypos / h);

        g_lastMousePos = glm::vec2(g_currInput->mouseX, g_currInput->mouseY);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
        
        if (action != GLFW_REPEAT) {
            b32 isPressed = action;
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                ProcessButton(g_currInput->mouseButtons[xjar::GameMouseInput_Left], isPressed);
            }
        }
    });
    
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (action != GLFW_REPEAT) {
            b32 isPressed = action;
            if (key == GLFW_KEY_W) {
                ProcessButton(g_currInput->actionUp, isPressed);
            }
            if (key == GLFW_KEY_S) {
                ProcessButton(g_currInput->actionDown, isPressed);
            }
            if (key == GLFW_KEY_D) {
                ProcessButton(g_currInput->actionRight, isPressed);
            }
            if (key == GLFW_KEY_A) {
                ProcessButton(g_currInput->actionLeft, isPressed);
            }
            if (key == GLFW_KEY_1) {
                ProcessButton(g_currInput->button1, isPressed);
            }
            if (key == GLFW_KEY_2) {
                ProcessButton(g_currInput->button2, isPressed);
            }

            if (key == GLFW_KEY_3) {
                ProcessButton(g_currInput->button3, isPressed);
            }

            if (mods & GLFW_MOD_SHIFT) {
                ProcessButton(g_currInput->actionAccelerate, isPressed);
            }

            if (key == GLFW_KEY_SPACE) {
                g_FpsCamera.SetUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
            }
        }
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int w, int h) {
        xjar::RenderSystem::Instance().OnResized(w, h);
    });


#if RENDERER_BACKEND == OpenGL
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    printf("Vendor %s\n", glGetString(GL_VENDOR));
    printf("Renderer %s\n", glGetString(GL_RENDERER));
    printf("Shading lang %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
#endif

    g_FpsCamera.Setup(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    auto &renderSystem = xjar::RenderSystem::Instance();
    auto &textureManager = xjar::TextureManager::Instance();
    renderSystem.Startup();

    f32 frameTime = static_cast<f32>(glfwGetTime());

    auto windowObj = xjar::GetWindow();
    
    auto         &world = xjar::World::Instance();
    xjar::Entity *ent = world.CreateEntity();
    xjar::Entity *ent2 = world.CreateEntity();
    xjar::Entity *ent3 = world.CreateEntity();


    renderSystem.LoadModel("assets/test.mesh", "assets/test.mesh.instance", "assets/test.materials", ent->model);
    renderSystem.LoadModel("assets/test.mesh", "assets/test.mesh.instance", "assets/test.materials", ent2->model);
    renderSystem.LoadModel("assets/test.mesh", "assets/test.mesh.instance", "assets/test.materials", ent3->model);

    memset(g_gameInput, 0, sizeof(xjar::GameInput));

    g_currInput = &g_gameInput[0];
    g_prevInput = &g_gameInput[1];

    while (!glfwWindowShouldClose(window)) {
    
        f32 currentTime = glfwGetTime();
        f32 dtForFrame = currentTime - static_cast<f32>(frameTime);

        frameTime = currentTime;

        memset(g_currInput, 0, sizeof(*g_currInput));
        for (int i = 0; i < xjar::BUTTON_COUNT; i++) {
            g_currInput->buttons[i].pressed = g_prevInput->buttons[i].pressed;
        }

        for (int i = 0; i < xjar::GameMouseInput_Count; i++) {
            g_currInput->mouseButtons[i].pressed = g_prevInput->mouseButtons[i].pressed;
        }

        glfwPollEvents();
        
        g_FpsCamera.Update(dtForFrame, g_currInput, g_lastMousePos);
        
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        ent->model.localTransform = model;

        glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
        ent2->model.localTransform = model2;
        
        glm::mat4 model3 = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 4.0f, 0.0f));
        ent3->model.localTransform = model3;

        xjar::GPU_SceneData sceneData{};
        sceneData.viewMat = g_FpsCamera.GetViewMatrix();
        sceneData.projMat = glm::perspective(glm::radians(45.0f), (f32)windowObj.width / (f32)windowObj.height, 0.1f, 1000.0f);
        sceneData.viewPos = g_FpsCamera.m_cameraPosition;

        auto frame = renderSystem.BeginFrame();
        if (frame.success) {

            renderSystem.BeginDefaultPass(&sceneData);
             // NOTE: here we should call features that use default render pass
            renderSystem.EndDefaultPass();
            
            renderSystem.BeginMultiMeshFeaturePass(frame);
            // NOTE: and here each feature will be used it's own render pass
            renderSystem.DrawEntities(frame, &sceneData, {ent, ent2, ent3});
            renderSystem.EndMultiMeshFeaturePass(frame);

            renderSystem.EndFrame();
        }

        
        xjar::GameInput *tempInput = g_currInput;
        g_currInput = g_prevInput;
        g_prevInput = tempInput;
    }


    renderSystem.Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
