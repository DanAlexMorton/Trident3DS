#include "texture_upscaler.hpp"
#include <algorithm>

namespace Trident {

void TextureUpscaler::init(ScaleFactor scale) {
    factor = scale;
}

void TextureUpscaler::reset() {
    factor = ScaleFactor::None;
}

std::vector<uint32_t> TextureUpscaler::upscale(const uint32_t* input, int width, int height) {
    int scale = static_cast<int>(factor);
    if (scale <= 1 || !input || width <= 0 || height <= 0) {
        return std::vector<uint32_t>(input, input + width * height);
    }

    int outW = width * scale;
    int outH = height * scale;
    std::vector<uint32_t> output(outW * outH);

    // Placeholder: nearest-neighbor upscale
    // Will be replaced with neural network inference
    for (int y = 0; y < outH; y++) {
        int srcY = y / scale;
        for (int x = 0; x < outW; x++) {
            int srcX = x / scale;
            output[y * outW + x] = input[srcY * width + srcX];
        }
    }

    return output;
}

} // namespace Trident
