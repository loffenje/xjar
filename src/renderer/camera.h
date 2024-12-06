#pragma once

#include <glm/mat4x4.hpp>

namespace xjar {

struct Camera {
    glm::mat4 view;
    glm::mat4 projection;
};

}

