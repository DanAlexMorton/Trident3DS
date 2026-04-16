#include "cpu.hpp"
#include "../memory/memory.hpp"

// TODO: Integrate Dynarmic here once submodule is wired up
// For now, this is a stub that will be replaced with real Dynarmic integration

namespace Trident {

struct CPU::Impl {
    Memory* memory = nullptr;
    SVCCallback svcHandler;

    uint32_t regs[16] = {};
    uint32_t cpsr = 0;
};

CPU::CPU() : impl(std::make_unique<Impl>()) {}
CPU::~CPU() = default;

bool CPU::init(Memory& memory) {
    impl->memory = &memory;
    reset();
    return true;
}

void CPU::reset() {
    std::fill(std::begin(impl->regs), std::end(impl->regs), 0);
    impl->cpsr = 0;
}

void CPU::setPC(uint32_t pc) { impl->regs[15] = pc; }
uint32_t CPU::getPC() const { return impl->regs[15]; }

void CPU::setReg(unsigned index, uint32_t value) {
    if (index < 16) impl->regs[index] = value;
}
uint32_t CPU::getReg(unsigned index) const {
    return (index < 16) ? impl->regs[index] : 0;
}

void CPU::setSP(uint32_t sp) { impl->regs[13] = sp; }
uint32_t CPU::getSP() const { return impl->regs[13]; }

void CPU::setCPSR(uint32_t cpsr) { impl->cpsr = cpsr; }
uint32_t CPU::getCPSR() const { return impl->cpsr; }

uint64_t CPU::run(uint64_t cycles) {
    // Stub — Dynarmic integration replaces this
    (void)cycles;
    return 0;
}

void CPU::setSVCHandler(SVCCallback callback) {
    impl->svcHandler = std::move(callback);
}

void CPU::invalidateCacheRange(uint32_t start, uint32_t size) {
    (void)start;
    (void)size;
}

void CPU::clearCache() {}

} // namespace Trident
