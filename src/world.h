#pragma once

#include "types.h"
#include "renderer/renderer_types.h"

namespace xjar {

enum class EntityType {
    Hero = 0 << 1,
};

struct Entity {
    EntityType type;
    b32        valid = false;
    Model      model;
};

static constexpr int MAX_ENTITY_COUNT = 1024;

struct World {
    Entity entities[MAX_ENTITY_COUNT];

    Entity *player;

    static World &Instance();

    World(const World &other) = delete;
    World &operator=(const World &other) = delete;

    Entity *CreateEntity();
    void    DestroyEntity(Entity *ent);

private:
    World() = default;
};

}
