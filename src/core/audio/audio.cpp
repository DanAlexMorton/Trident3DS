#include "audio.hpp"
#include <cstring>

namespace Trident {

bool Audio::init(Memory& mem) {
    memory = &mem;
    return true;
}

void Audio::reset() {}

size_t Audio::generateSamples(int16_t* buffer, size_t maxSamples) {
    // Stub: generate silence
    size_t count = std::min(maxSamples, static_cast<size_t>(SAMPLES_PER_FRAME));
    std::memset(buffer, 0, count * 2 * sizeof(int16_t)); // stereo
    return count;
}

} // namespace Trident
