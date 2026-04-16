#include "cpu.hpp"
#include "../memory/memory.hpp"

#include <algorithm>
#include <cstdio>

#ifdef TRIDENT_USE_DYNARMIC
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/config.h"
#endif

namespace Trident {

#ifdef TRIDENT_USE_DYNARMIC
// DynarmicCallbacks bridges Dynarmic's memory/SVC/timing callbacks to our Memory bus and kernel.
// Lives entirely inside cpu.cpp — never exposed in any header.
struct DynarmicCallbacks : public Dynarmic::A32::UserCallbacks {
    Memory* mem = nullptr;
    CPU::SVCCallback svcHandler;
    uint64_t ticksRemaining = 0;

    // --- Memory reads ---
    uint8_t  MemoryRead8 (uint32_t vaddr) override { return mem->read8(vaddr); }
    uint16_t MemoryRead16(uint32_t vaddr) override { return mem->read16(vaddr); }
    uint32_t MemoryRead32(uint32_t vaddr) override { return mem->read32(vaddr); }
    uint64_t MemoryRead64(uint32_t vaddr) override { return mem->read64(vaddr); }

    // --- Memory writes ---
    void MemoryWrite8 (uint32_t vaddr, uint8_t  value) override { mem->write8(vaddr, value); }
    void MemoryWrite16(uint32_t vaddr, uint16_t value) override { mem->write16(vaddr, value); }
    void MemoryWrite32(uint32_t vaddr, uint32_t value) override { mem->write32(vaddr, value); }
    void MemoryWrite64(uint32_t vaddr, uint64_t value) override { mem->write64(vaddr, value); }

    // --- SVC dispatch ---
    void CallSVC(uint32_t swi) override {
        if (svcHandler) {
            svcHandler(swi);
        } else {
            fprintf(stderr, "[CPU] SVC 0x%02X called but no handler installed\n", swi);
        }
    }

    // --- Exception handling ---
    void ExceptionRaised(uint32_t pc, Dynarmic::A32::Exception exception) override {
        fprintf(stderr, "[CPU] Exception at PC=0x%08X: %d\n", pc, static_cast<int>(exception));
    }

    // --- Interpreter fallback (for rare/unimplemented instructions) ---
    void InterpreterFallback(uint32_t pc, size_t num_instructions) override {
        fprintf(stderr, "[CPU] InterpreterFallback at PC=0x%08X (%zu instructions) — not implemented\n",
                pc, num_instructions);
    }

    // --- Timing ---
    void AddTicks(uint64_t ticks) override {
        ticksRemaining = (ticks >= ticksRemaining) ? 0 : ticksRemaining - ticks;
    }
    uint64_t GetTicksRemaining() override { return ticksRemaining; }
};
#endif // TRIDENT_USE_DYNARMIC

struct CPU::Impl {
    Memory* memory = nullptr;
    SVCCallback svcHandler;

#ifdef TRIDENT_USE_DYNARMIC
    std::unique_ptr<DynarmicCallbacks> callbacks;
    std::unique_ptr<Dynarmic::A32::Jit> jit;
#endif

    // Fallback register state used when Dynarmic is not available
    uint32_t regs[16] = {};
    uint32_t cpsr = 0;
};

CPU::CPU() : impl(std::make_unique<Impl>()) {}
CPU::~CPU() = default;

bool CPU::init(Memory& memory) {
    impl->memory = &memory;

#ifdef TRIDENT_USE_DYNARMIC
    impl->callbacks = std::make_unique<DynarmicCallbacks>();
    impl->callbacks->mem = &memory;

    Dynarmic::A32::UserConfig config;
    config.callbacks = impl->callbacks.get();
    config.arch_version = Dynarmic::A32::ArchVersion::v6K; // ARM11 = ARMv6K, not v8

    impl->jit = std::make_unique<Dynarmic::A32::Jit>(config);
    fprintf(stderr, "[CPU] Dynarmic JIT initialised (ARMv6K)\n");
#else
    fprintf(stderr, "[CPU] Running stub CPU — Dynarmic not compiled in\n");
#endif

    reset();
    return true;
}

void CPU::reset() {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) {
        impl->jit->ClearCache();
        impl->jit->Regs().fill(0);
        impl->jit->SetCpsr(0x00000010); // User mode (M=10000), ARM state (T=0)
    }
#endif
    std::fill(std::begin(impl->regs), std::end(impl->regs), 0u);
    impl->cpsr = 0x00000010;
}

void CPU::setPC(uint32_t pc) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) { impl->jit->Regs()[15] = pc; return; }
#endif
    impl->regs[15] = pc;
}

uint32_t CPU::getPC() const {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) return impl->jit->Regs()[15];
#endif
    return impl->regs[15];
}

void CPU::setReg(unsigned index, uint32_t value) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit && index < 16) { impl->jit->Regs()[index] = value; return; }
#endif
    if (index < 16) impl->regs[index] = value;
}

uint32_t CPU::getReg(unsigned index) const {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit && index < 16) return impl->jit->Regs()[index];
#endif
    return (index < 16) ? impl->regs[index] : 0u;
}

void CPU::setSP(uint32_t sp) { setReg(13, sp); }
uint32_t CPU::getSP() const { return getReg(13); }

void CPU::setCPSR(uint32_t cpsr) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) { impl->jit->SetCpsr(cpsr); return; }
#endif
    impl->cpsr = cpsr;
}

uint32_t CPU::getCPSR() const {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) return impl->jit->Cpsr();
#endif
    return impl->cpsr;
}

uint64_t CPU::run(uint64_t cycles) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) {
        // Keep the svcHandler in sync in case it was updated after init()
        impl->callbacks->svcHandler = impl->svcHandler;
        impl->callbacks->ticksRemaining = cycles;
        impl->jit->Run();
        return cycles - impl->callbacks->ticksRemaining;
    }
#endif
    (void)cycles;
    return 0;
}

void CPU::setSVCHandler(SVCCallback callback) {
    impl->svcHandler = std::move(callback);
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->callbacks) impl->callbacks->svcHandler = impl->svcHandler;
#endif
}

void CPU::invalidateCacheRange(uint32_t start, uint32_t size) {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) impl->jit->InvalidateCacheRange(start, size);
#endif
    (void)start;
    (void)size;
}

void CPU::clearCache() {
#ifdef TRIDENT_USE_DYNARMIC
    if (impl->jit) impl->jit->ClearCache();
#endif
}

} // namespace Trident
