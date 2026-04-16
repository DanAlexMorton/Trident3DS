#include "kernel.hpp"
#include "../cpu/cpu.hpp"
#include "../memory/memory.hpp"
#include <cstdio>

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
    switch (svc) {
        case 0x01: { // svcControlMemory
            // r0 = op, r1 = addr0, r2 = addr1, r3 = size, (stack) = perms
            // Phase 1 stub: return a fake heap address in r1 and success in r0
            impl->cpu->setReg(1, 0x08000000); // fake heap address
            impl->cpu->setReg(0, 0);           // success
            break;
        }

        case 0x08: // svcCreateThread
        case 0x09: // svcExitThread
        case 0x0A: // svcSleepThread
        case 0x13: // svcCreateMutex
        case 0x17: // svcCreateEvent
        case 0x23: // svcCloseHandle
        case 0x24: // svcWaitSynchronization1
        case 0x32: { // svcSendSyncRequest
            fprintf(stderr, "[KERNEL] SVC 0x%02X (stubbed)\n", svc);
            impl->cpu->setReg(0, 0); // success
            break;
        }

        case 0x3C: { // svcBreak
            // r0 = reason (0=PANIC, 1=ASSERT, 2=USER)
            uint32_t reason = impl->cpu->getReg(0);
            const char* reasons[] = {"PANIC", "ASSERT", "USER"};
            fprintf(stderr, "[KERNEL] svcBreak: %s\n",
                    reason < 3 ? reasons[reason] : "UNKNOWN");
            // Signal halt by clearing the SVC handler so CPU stops calling back
            impl->cpu->setSVCHandler(nullptr);
            impl->cpu->setReg(0, 0); // success
            break;
        }

        case 0x3D: { // svcOutputDebugString
            // r0 = string ptr (virtual address), r1 = length
            uint32_t ptr = impl->cpu->getReg(0);
            uint32_t len = impl->cpu->getReg(1);
            for (uint32_t i = 0; i < len && i < 256; i++) {
                fputc(impl->memory->read8(ptr + i), stderr);
            }
            fputc('\n', stderr);
            impl->cpu->setReg(0, 0); // success
            break;
        }

        default:
            fprintf(stderr, "[KERNEL] Unimplemented SVC: 0x%02X at PC=0x%08X\n",
                    svc, impl->cpu->getPC());
            impl->cpu->setReg(0, 0); // return success to avoid crashes
            break;
    }
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
