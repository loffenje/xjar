#pragma once

#include "types.h"

namespace xjar {

struct MaterialDescr {
    gpuvec4 albedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
    gpuvec4 emissiveColor = {1.0f, 1.0f, 1.0f, 1.0f};
    u64     diffuseMap = INVALID_TEXTURE;
    u64     specularMap = INVALID_TEXTURE;
    u64     normalMap = INVALID_TEXTURE;
    u32     padding = 0;
};

static_assert(sizeof(MaterialDescr) % 16 == 0, "MaterialDescr should be padded to 16 bytes");

}
