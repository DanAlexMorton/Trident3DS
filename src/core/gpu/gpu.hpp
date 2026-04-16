#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace Trident {

class Memory;

// PICA200 GPU emulation stub
class GPU {
public:
    GPU();
    ~GPU();

    bool init(Memory& memory);
    void reset();

    // Process GPU command list
    void processCommandList(uint32_t addr, uint32_t size);

    // Get framebuffer for display
    uint32_t getTopFramebufferAddr() const;
    uint32_t getBottomFramebufferAddr() const;

    // Screen dimensions
    static constexpr unsigned TOP_SCREEN_WIDTH = 400;
    static constexpr unsigned TOP_SCREEN_HEIGHT = 240;
    static constexpr unsigned BOTTOM_SCREEN_WIDTH = 320;
    static constexpr unsigned BOTTOM_SCREEN_HEIGHT = 240;

    // Combined output (both screens stacked)
    static constexpr unsigned OUTPUT_WIDTH = 400;
    static constexpr unsigned OUTPUT_HEIGHT = 480; // 240 + 240

    // Render a frame — fills outputBuffer with XRGB8888
    void renderFrame(uint32_t* outputBuffer);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Trident
