#include "kernel.hpp"
#include "../cpu/cpu.hpp"
#include "../memory/memory.hpp"

namespace Trident {

struct Kernel::Impl {
    CPU* cpu = nullptr;
    Memory* memory = nullptr;
};

Kernel::Kernel() : impl(std::make_unique<Impl>()) {}
Kernel::~Kernel() = default;

bool Kernel::init(CPU& cpu, Memory& memory) {
    impl->cpu = &cpu;
    impl->memory = &memory;
    return true;
}

void Kernel::reset() {}

void Kernel::handleSVC(uint32_t svc) {
    // TODO: Implement SVCs
    // Key SVCs needed:
    // 0x01 - ControlMemory
    // 0x02 - QueryMemory
    // 0x08 - CreateThread
    // 0x09 - ExitThread
    // 0x0A - SleepThread
    // 0x13 - CreateMutex
    // 0x17 - CreateEvent
    // 0x1F - MapMemoryBlock
    // 0x21 - CreateAddressArbiter
    // 0x23 - CloseHandle
    // 0x24 - WaitSynchronization1
    // 0x25 - WaitSynchronizationN
    // 0x32 - SendSyncRequest
    // 0x35 - GetProcessId
    // 0x37 - GetResourceLimit
    // 0x38 - GetResourceLimitLimitValues
    // 0x3D - OutputDebugString
    (void)svc;
}

void Kernel::createThread(uint32_t entryPoint, uint32_t stackTop, uint32_t priority) {
    (void)entryPoint;
    (void)stackTop;
    (void)priority;
}

void Kernel::schedule() {}

void Kernel::svcControlMemory(uint32_t op, uint32_t addr0, uint32_t addr1, uint32_t size, uint32_t perms) {
    (void)op; (void)addr0; (void)addr1; (void)size; (void)perms;
}

void Kernel::svcMapMemoryBlock(uint32_t handle, uint32_t addr, uint32_t perms) {
    (void)handle; (void)addr; (void)perms;
}

} // namespace Trident
