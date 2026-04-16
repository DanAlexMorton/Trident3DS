#pragma once

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "kernel/kernel.hpp"
#include "gpu/gpu.hpp"
#include "audio/audio.hpp"
#include "loader/loader.hpp"
#include "patcher/patcher.hpp"

#include <memory>
#include <string>
#include <functional>

namespace Trident {

class Emulator {
public:
    Emulator();
    ~Emulator();

    bool init(bool isNew3DS = false);
    void reset();
    void shutdown();

    bool loadROM(const std::string& path);
    bool applyPatch(const std::string& patchPath, const std::string& romPath);

    void runFrame();
    void renderFrame(uint32_t* output);
    size_t generateAudio(int16_t* buffer, size_t maxSamples);

    // Input
    void setButton(unsigned id, bool pressed);
    void setCirclePad(int16_t x, int16_t y);
    void setTouchScreen(int16_t x, int16_t y, bool touching);

    // State
    bool isRunning() const { return running; }
    uint32_t getEntryPoint() const { return entryPoint; }
    uint64_t getProgramId() const { return programId; }

    // Component access
    CPU& getCPU() { return *cpu; }
    Memory& getMemory() { return *memory; }
    Kernel& getKernel() { return *kernel; }
    GPU& getGPU() { return *gpu; }
    Audio& getAudio() { return *audio; }

private:
    std::unique_ptr<CPU> cpu;
    std::unique_ptr<Memory> memory;
    std::unique_ptr<Kernel> kernel;
    std::unique_ptr<GPU> gpu;
    std::unique_ptr<Audio> audio;

    bool running = false;
    bool isNew3DS = false;
    uint32_t entryPoint = 0;
    uint64_t programId = 0;

    // HID state
    uint32_t buttonState = 0;
    int16_t circlePadX = 0;
    int16_t circlePadY = 0;
    int16_t touchX = 0;
    int16_t touchY = 0;
    bool touchActive = false;

    static constexpr uint64_t CPU_CLOCK_RATE = 268111856;
    static constexpr uint64_t CYCLES_PER_FRAME = CPU_CLOCK_RATE / 60;

    void setupMemoryMap();
    void updateHIDRegisters();
};

} // namespace Trident
