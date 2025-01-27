#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include "types.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <array>
#include <glm/mat4x4.hpp>
#include <vector>
#include <optional>
#include <span>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <iostream>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <string_view>
#include <assert.h>
#include <string.h>
#include <unordered_set>

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#if RENDERER_BACKEND == OpenGL
#include "glfw_gl.h"
#else
#include "glfw_vk.h"
#endif
