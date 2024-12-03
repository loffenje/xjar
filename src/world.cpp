#include "world.h"
#include <assert.h>
#include <string.h>

namespace xjar {

World &World::Instance() {
    static World world;

    return world;
}

Entity *World::CreateEntity() {
    Entity *result = nullptr;
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        Entity *ent = &entities[i];
        if (!ent->valid) {
            ent->valid = true;

            result = ent;
            return result;
        }
    }

    assert(!"No free entity");
    return nullptr;
}

void World::DestroyEntity(Entity *ent) {
    memset(ent, 0, sizeof(*ent));
}

}
