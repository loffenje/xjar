#pragma once

#include "types.h"
#include <string>

namespace xjar {

struct Texture {
    u32          id = 0;
    u32          width;
    u32          height;
    int          nr;
    std::string  name;
    void *       handle;
};

}
