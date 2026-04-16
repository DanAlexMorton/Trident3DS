#pragma once

#include <cstdint>
#include <vector>

namespace Trident {

// Placeholder for AI-based texture upscaling.
// Future implementation will use a lightweight neural network (e.g., ESPCN or similar)
// to upscale 3DS textures in real-time during rendering.
class TextureUpscaler {
public:
    enum class ScaleFactor { None = 1, X2 = 2, X4 = 4 };

    void init(ScaleFactor scale = ScaleFactor::None);
    void reset();

    bool isEnabled() const { return factor != ScaleFactor::None; }
    int getScaleFactor() const { return static_cast<int>(factor); }

    // Upscale an RGBA8888 texture. Returns upscaled data.
    // Input: width x height RGBA pixels. Output: (width*scale) x (height*scale) RGBA pixels.
    std::vector<uint32_t> upscale(const uint32_t* input, int width, int height);

private:
    ScaleFactor factor = ScaleFactor::None;
};

} // namespace Trident
