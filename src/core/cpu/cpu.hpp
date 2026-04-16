#pragma once

#include <cstdint>
#include <memory>
#include <functional>

namespace Trident {

class Memory;

class CPU {
public:
    using ReadCallback8  = std::function<uint8_t(uint32_t)>;
    using ReadCallback16 = std::function<uint16_t(uint32_t)>;
    using ReadCallback32 = std::function<uint32_t(uint32_t)>;
    using WriteCallback8  = std::function<void(uint32_t, uint8_t)>;
    using WriteCallback16 = std::function<void(uint32_t, uint16_t)>;
    using WriteCallback32 = std::function<void(uint32_t, uint32_t)>;
    using SVCCallback = std::function<void(uint32_t)>;

    CPU();
    ~CPU();

    bool init(Memory& memory);
    void reset();

    void setPC(uint32_t pc);
    uint32_t getPC() const;

    void setReg(unsigned index, uint32_t value);
    uint32_t getReg(unsigned index) const;

    void setSP(uint32_t sp);
    uint32_t getSP() const;

    void setCPSR(uint32_t cpsr);
    uint32_t getCPSR() const;

    // Execute N cycles, returns cycles actually executed
    uint64_t run(uint64_t cycles);

    // Install SVC handler
    void setSVCHandler(SVCCallback callback);

    // Cache invalidation
    void invalidateCacheRange(uint32_t start, uint32_t size);
    void clearCache();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Trident
