#pragma once

#include <cstdint>

namespace Trident {

class Memory;

// Audio DSP stub (HLE)
class Audio {
public:
    Audio() = default;
    ~Audio() = default;

    bool init(Memory& memory);
    void reset();

    // Generate audio samples for one frame
    // Returns number of stereo samples generated
    size_t generateSamples(int16_t* buffer, size_t maxSamples);

    static constexpr unsigned SAMPLE_RATE = 32768;
    static constexpr unsigned SAMPLES_PER_FRAME = SAMPLE_RATE / 60; // ~546

private:
    Memory* memory = nullptr;
};

} // namespace Trident
