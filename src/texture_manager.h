#pragma once

#include "types.h"
#include "renderer/resource_types.h"
#include <optional>
#include <vector>
#include <unordered_map>

namespace xjar {

struct TextureRef {
    Texture texture;
    u32     refcount;
    b32     autorelease;
};

class TextureManager {
public:
    static TextureManager &Instance();

    void           StartUp(u32 maxTextureNum);
    void           Shutdown();
    const Texture &GetDefaultTexture();
    const Texture &Acquire(const std::string &name, b32 autorelease = true);
    void           Release(const std::string &name);
    static std::optional<Texture> LoadTexture(const std::string &textureName);
    TextureManager(const TextureManager &) = delete;
    TextureManager &operator=(const TextureManager &) = delete;

private:
    TextureManager() = default;

    std::unordered_map<std::string, TextureRef> m_textures;
};

}
