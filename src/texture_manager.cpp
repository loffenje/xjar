#include "texture_manager.h"

#include "renderer/resource_types.h"
#include "renderer/render_system.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <array>

namespace xjar {

Texture g_defaultTexture;

static void CreateDefaultTexture() {
    constexpr u32 textureSize = 16;
    constexpr u32 channels = 4;
    constexpr u32 pixelCount = textureSize * textureSize;

    std::array<u8, pixelCount * channels> pixels;

    std::fill(pixels.begin(), pixels.end(), 0);

    // Each pixel.
    for (u32 row = 0; row < textureSize; ++row) {
        for (u32 col = 0; col < textureSize; ++col) {
            u32 index = (row * textureSize) + col;
            u32 index_bpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
        }
    }
    g_defaultTexture.name = "texture.png";
    g_defaultTexture.width = textureSize;
    g_defaultTexture.height = textureSize;
    g_defaultTexture.id = 0;
    g_defaultTexture.nr = channels;

    RenderSystem::Instance().CreateTexture(pixels.data(), &g_defaultTexture);
    g_defaultTexture.id = INVALID_TEXTURE;
}

std::optional<Texture> TextureManager::LoadTexture(const std::string &textureName) {
    int      w, h, nr;
    stbi_uc *pixels = stbi_load(textureName.c_str(), &w, &h, &nr, STBI_rgb_alpha);

    if (!pixels) {
        fprintf(stderr, "Failed to read file %s\n", textureName.c_str());
        return {};
    }

    Texture texture {};
    texture.nr = nr;
    texture.width = static_cast<u32>(w);
    texture.height = static_cast<u32>(h);
    texture.name = textureName;

    RenderSystem::Instance().CreateTexture(pixels, &texture);

    stbi_image_free(pixels);

    return texture;
}

TextureManager &TextureManager::Instance() {
    static TextureManager manager;

    return manager;
}

void TextureManager::StartUp(u32 maxTextureNum) {
    assert(maxTextureNum > 0);

    m_textures.reserve(maxTextureNum);

    CreateDefaultTexture();
}

const Texture &TextureManager::GetDefaultTexture() {
    return g_defaultTexture;
}

const Texture &TextureManager::Acquire(const std::string &name, b32 autorelease) {
    if (m_textures.contains(name)) {
        TextureRef &textureRef = m_textures[name];
        ++textureRef.refcount;

        return textureRef.texture;
    }

    auto texture = TextureManager::LoadTexture(name);
    if (texture.has_value()) {
        TextureRef textureRef {.texture = *texture, .refcount = 0, .autorelease = autorelease};
        m_textures[name] = textureRef;

        return textureRef.texture;
    }

    Texture invalid {};

    return invalid;
}

void TextureManager::Release(const std::string &name) {
    if (m_textures.contains(name)) {
        TextureRef &textureRef = m_textures[name];

        --textureRef.refcount;

        if (textureRef.refcount == 0 && textureRef.autorelease) {
            RenderSystem::Instance().DestroyTexture(&textureRef.texture);

            textureRef.autorelease = false;
        }
    } else {
        fprintf(stderr, "Failed to release texture %s\n", name.c_str());
    }
}

}
