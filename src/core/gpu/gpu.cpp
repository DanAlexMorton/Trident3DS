#include "gpu.hpp"
#include "../memory/memory.hpp"
#include <cstring>

namespace Trident {

struct GPU::Impl {
    Memory* memory = nullptr;
    uint32_t topFBAddr = 0;
    uint32_t bottomFBAddr = 0;
};

GPU::GPU() : impl(std::make_unique<Impl>()) {}
GPU::~GPU() = default;

bool GPU::init(Memory& memory) {
    impl->memory = &memory;
    return true;
}

void GPU::reset() {
    impl->topFBAddr = 0;
    impl->bottomFBAddr = 0;
}

void GPU::processCommandList(uint32_t addr, uint32_t size) {
    // TODO: Parse PICA200 command buffer
    // Key registers:
    // 0x01C - Viewport
    // 0x040-0x047 - Framebuffer config
    // 0x050-0x05F - Rasterizer
    // 0x080-0x0FF - Texture config
    // 0x100-0x1FF - Fragment lighting
    // 0x200-0x27F - Geometry pipeline
    // 0x280-0x2DF - Fragment shader
    (void)addr;
    (void)size;
}

uint32_t GPU::getTopFramebufferAddr() const { return impl->topFBAddr; }
uint32_t GPU::getBottomFramebufferAddr() const { return impl->bottomFBAddr; }

void GPU::renderFrame(uint32_t* outputBuffer) {
    // Stub: output solid dark blue to indicate the GPU is alive
    for (unsigned i = 0; i < OUTPUT_WIDTH * OUTPUT_HEIGHT; ++i) {
        outputBuffer[i] = 0xFF1A1A2E; // Dark navy
    }
}

} // namespace Trident
