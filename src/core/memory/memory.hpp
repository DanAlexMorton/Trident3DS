#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace Trident {

// 3DS memory regions
constexpr uint32_t FCRAM_SIZE = 128 * 1024 * 1024;        // 128 MB FCRAM
constexpr uint32_t FCRAM_N3DS_SIZE = 256 * 1024 * 1024;   // 256 MB on New 3DS
constexpr uint32_t VRAM_SIZE = 6 * 1024 * 1024;           // 6 MB VRAM
constexpr uint32_t DSP_MEM_SIZE = 512 * 1024;             // 512 KB DSP memory
constexpr uint32_t AXI_WRAM_SIZE = 512 * 1024;            // 512 KB AXI WRAM

// Virtual address ranges (ARM11 userland)
constexpr uint32_t VADDR_CODE_START      = 0x00100000;
constexpr uint32_t VADDR_CODE_END        = 0x04000000;
constexpr uint32_t VADDR_HEAP_START      = 0x08000000;
constexpr uint32_t VADDR_HEAP_END        = 0x10000000;
constexpr uint32_t VADDR_SHARED_START    = 0x10000000;
constexpr uint32_t VADDR_SHARED_END      = 0x14000000;
constexpr uint32_t VADDR_LINEAR_START    = 0x14000000;
constexpr uint32_t VADDR_LINEAR_END      = 0x1C000000;
constexpr uint32_t VADDR_N3DS_EXTRA      = 0x1E800000;
constexpr uint32_t VADDR_N3DS_EXTRA_END  = 0x1EC00000;
constexpr uint32_t VADDR_VRAM_START      = 0x1F000000;
constexpr uint32_t VADDR_VRAM_END        = 0x1F600000;
constexpr uint32_t VADDR_DSP_START       = 0x1FF00000;
constexpr uint32_t VADDR_DSP_END         = 0x1FF80000;
constexpr uint32_t VADDR_TLS_START       = 0x1FF82000;
constexpr uint32_t VADDR_TLS_END         = 0x20000000;
constexpr uint32_t VADDR_NEW_LINEAR      = 0x30000000;
constexpr uint32_t VADDR_NEW_LINEAR_END  = 0x40000000;

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t PAGE_BITS = 12;
constexpr size_t PAGE_TABLE_SIZE = 0x100000; // 1M entries for 4GB address space

enum class PageFlags : uint8_t {
    Unmapped = 0,
    Read     = 1 << 0,
    Write    = 1 << 1,
    Execute  = 1 << 2,
    ReadWrite = Read | Write,
    All      = Read | Write | Execute,
};

struct PageEntry {
    uint8_t* ptr = nullptr;          // Host pointer to backing memory
    PageFlags flags = PageFlags::Unmapped;
};

class Memory {
public:
    Memory();
    ~Memory();

    bool init(bool isNew3DS = false);
    void reset();

    // Read/write by virtual address
    uint8_t  read8(uint32_t vaddr);
    uint16_t read16(uint32_t vaddr);
    uint32_t read32(uint32_t vaddr);
    uint64_t read64(uint32_t vaddr);

    void write8(uint32_t vaddr, uint8_t value);
    void write16(uint32_t vaddr, uint16_t value);
    void write32(uint32_t vaddr, uint32_t value);
    void write64(uint32_t vaddr, uint64_t value);

    // Bulk operations
    void readBlock(uint32_t vaddr, void* dest, size_t size);
    void writeBlock(uint32_t vaddr, const void* src, size_t size);

    // Page table management
    void mapPages(uint32_t vaddr, uint8_t* hostPtr, size_t size, PageFlags flags);
    void unmapPages(uint32_t vaddr, size_t size);

    // Direct access (for DMA, GPU, RetroAchievements)
    uint8_t* getPointer(uint32_t vaddr);
    const uint8_t* getPointer(uint32_t vaddr) const;

    // Backing memory access
    uint8_t* getFCRAM() { return fcram.data(); }
    uint8_t* getVRAM() { return vram.data(); }
    size_t getFCRAMSize() const { return fcram.size(); }

    // RetroAchievements memory descriptor generation
    struct MemoryRegion {
        uint32_t start;
        size_t   length;
        uint8_t* pointer;
    };
    std::vector<MemoryRegion> getContiguousMappedRegions() const;

private:
    std::vector<uint8_t> fcram;
    std::vector<uint8_t> vram;
    std::vector<uint8_t> dspMem;

    std::array<PageEntry, PAGE_TABLE_SIZE> pageTable;
    bool n3dsMode = false;

    PageEntry& getPage(uint32_t vaddr) {
        return pageTable[vaddr >> PAGE_BITS];
    }
    const PageEntry& getPage(uint32_t vaddr) const {
        return pageTable[vaddr >> PAGE_BITS];
    }
};

} // namespace Trident
