#pragma once

#include <cstdint>
#include <memory>

namespace Trident {

class Memory;
class CPU;

class Kernel {
public:
    Kernel();
    ~Kernel();

    bool init(CPU& cpu, Memory& memory);
    void reset();

    // Handle SVC (supervisor call) from CPU
    void handleSVC(uint32_t svc);

    // Threading
    void createThread(uint32_t entryPoint, uint32_t stackTop, uint32_t priority);
    void schedule();

    // Memory management SVCs
    void svcControlMemory(uint32_t op, uint32_t addr0, uint32_t addr1, uint32_t size, uint32_t perms);
    void svcMapMemoryBlock(uint32_t handle, uint32_t addr, uint32_t perms);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Trident
