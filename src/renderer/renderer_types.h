#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include "types.h"

#include "resource_types.h"

namespace xjar {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texcoord;
};

struct Mesh {
    u32                 id;
    std::vector<Vertex> vertices;
    std::vector<u32>    indices;
};

struct Model {
    Texture           texture;
    glm::mat4         localTransform;
    std::vector<Mesh> meshes;
};

}
