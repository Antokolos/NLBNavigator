#include "ImageConvert.h"

#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

bool convertPngToJpg(const std::string& png, int quality, bool transparentToMagenta, std::string& jpg) {
    int w = 0, h = 0, n = 0;
    unsigned char* rgba = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(png.data()), static_cast<int>(png.size()), &w, &h, &n, 4);
    if (!rgba) return false;
    std::vector<unsigned char> rgb(static_cast<size_t>(w) * h * 3);
    bool transparent = false;
    const unsigned bg[3] = {255, 0, 255};  // фон прозрачного, как в NLBB
    for (size_t i = 0, px = static_cast<size_t>(w) * h; i < px; ++i) {
        const unsigned a = rgba[i * 4 + 3];
        if (a < 255) transparent = true;
        for (int c = 0; c < 3; ++c) rgb[i * 3 + c] = static_cast<unsigned char>((rgba[i * 4 + c] * a + bg[c] * (255 - a)) / 255);
    }
    stbi_image_free(rgba);
    if (transparent && !transparentToMagenta) return false;  // спрайт с прозрачностью — оставить PNG
    jpg.clear();
    auto write = [](void* ctx, void* data, int size) {
        static_cast<std::string*>(ctx)->append(static_cast<const char*>(data), static_cast<size_t>(size));
    };
    return stbi_write_jpg_to_func(write, &jpg, w, h, 3, rgb.data(), quality) != 0 && !jpg.empty();
}
