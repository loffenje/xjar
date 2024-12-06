#pragma once

#include <initializer_list>
#include "renderer_types.h"
#include "camera.h"

namespace xjar {

struct Entity;

class TestFeature {
public:
    virtual void Init(void *device, void *renderPass) = 0;
    virtual void DrawEntities(void *cmdbuf, const Camera &camera, std::initializer_list<Entity *> entities) = 0;

    virtual ~TestFeature() = default;
};

}

