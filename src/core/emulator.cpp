#include "emulator.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace Trident {

Emulator::Emulator() = default;
Emulator::~Emulator() { shutdown(); }

bool Emulator::init(bool newModel) {
    isNew3DS = newModel;

    cpu = std::make_unique<CPU>();
    memory = std::make_unique<Memory>();
    kernel = std::make_unique<Kernel>();
    gpu = std::make_unique<GPU>();
    audio = std::make_unique<Audio>();

    memory->init(isNew3DS);
    cpu->init(*memory);
    kernel->init(*cpu, *memory);
    gpu->init(*memory);
    audio->init(*memory);

    setupMemoryMap();
    return true;
}

void Emulator::reset() {
    cpu->reset();
    memory->reset();
    kernel->reset();
    gpu->reset();
    audio->reset();

    running = false;
    entryPoint = 0;
    programId = 0;
    buttonState = 0;
    circlePadX = circlePadY = 0;
    touchX = touchY = 0;
    touchActive = false;

    memory->init(isNew3DS);
    setupMemoryMap();
}

void Emulator::shutdown() {
    running = false;
    cpu.reset();
    memory.reset();
    kernel.reset();
    gpu.reset();
    audio.reset();
}

void Emulator::setupMemoryMap() {
    // Map application code region
    memory->mapPages(VADDR_CODE_START, memory->getFCRAM(),
                     VADDR_CODE_END - VADDR_CODE_START,
                     PageFlags::All);

    // Map heap region
    memory->mapPages(VADDR_HEAP_START,
                     memory->getFCRAM() + (VADDR_CODE_END - VADDR_CODE_START),
                     VADDR_HEAP_END - VADDR_HEAP_START,
                     PageFlags::ReadWrite);

    // Map VRAM
    memory->mapPages(VADDR_VRAM_START, memory->getVRAM(),
                     VRAM_SIZE, PageFlags::ReadWrite);
}

bool Emulator::loadROM(const std::string& path) {
    auto result = Loader::loadFile(path, *memory);
    if (!result.success) return false;

    entryPoint = result.entryPoint;
    programId = result.programId;

    cpu->setPC(entryPoint);
    cpu->setSP(VADDR_HEAP_END - 0x1000);
    cpu->setSVCHandler([this](uint32_t svc) { kernel->handleSVC(svc); });

    running = true;
    return true;
}

bool Emulator::applyPatch(const std::string& patchPath, const std::string& romPath) {
    // Read ROM file from disk
    std::ifstream romFile(romPath, std::ios::binary | std::ios::ate);
    if (!romFile.is_open()) return false;
    auto fileSize = romFile.tellg();
    romFile.seekg(0);
    std::vector<uint8_t> romData(static_cast<size_t>(fileSize));
    romFile.read(reinterpret_cast<char*>(romData.data()), fileSize);

    // Apply patch in-place using the Patcher API
    if (!Patcher::apply(romData, patchPath)) return false;

    // Write patched data back to FCRAM at code start
    size_t copySize = std::min(romData.size(),
                               static_cast<size_t>(VADDR_CODE_END - VADDR_CODE_START));
    std::memcpy(memory->getFCRAM(), romData.data(), copySize);
    return true;
}

void Emulator::runFrame() {
    if (!running) return;

    updateHIDRegisters();
    cpu->run(CYCLES_PER_FRAME);
}

void Emulator::renderFrame(uint32_t* output) {
    if (!running || !gpu) return;
    gpu->renderFrame(output);
}

size_t Emulator::generateAudio(int16_t* buffer, size_t maxSamples) {
    if (!running || !audio) return 0;
    return audio->generateSamples(buffer, maxSamples);
}

void Emulator::setButton(unsigned id, bool pressed) {
    if (pressed)
        buttonState |= (1u << id);
    else
        buttonState &= ~(1u << id);
}

void Emulator::setCirclePad(int16_t x, int16_t y) {
    circlePadX = x;
    circlePadY = y;
}

void Emulator::setTouchScreen(int16_t x, int16_t y, bool touching) {
    touchX = x;
    touchY = y;
    touchActive = touching;
}

void Emulator::updateHIDRegisters() {
    // TODO: Write button/touch/circle pad state to HID shared memory
    // HID shared memory at 0x10146000
    // Offset 0x1C: pad state (inverted: 0 = pressed)
    // Offset 0x48: circle pad (x: bits 0-11, y: bits 12-23)
    // Offset 0xA8: touch (x: bits 0-15, y: bits 16-31, valid: bit 0 of next word)
}

} // namespace Trident
