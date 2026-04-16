#include "memory.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Trident {

Memory::Memory() = default;
Memory::~Memory() = default;

bool Memory::init(bool isNew3DS) {
    n3dsMode = isNew3DS;
    fcram.resize(isNew3DS ? FCRAM_N3DS_SIZE : FCRAM_SIZE, 0);
    vram.resize(VRAM_SIZE, 0);
    dspMem.resize(DSP_MEM_SIZE, 0);
    pageTable.fill(PageEntry{});
    return true;
}

void Memory::reset() {
    std::fill(fcram.begin(), fcram.end(), 0);
    std::fill(vram.begin(), vram.end(), 0);
    std::fill(dspMem.begin(), dspMem.end(), 0);
    pageTable.fill(PageEntry{});
}

void Memory::mapPages(uint32_t vaddr, uint8_t* hostPtr, size_t size, PageFlags flags) {
    const size_t numPages = size >> PAGE_BITS;
    for (size_t i = 0; i < numPages; ++i) {
        auto& page = pageTable[(vaddr >> PAGE_BITS) + i];
        page.ptr = hostPtr + (i * PAGE_SIZE);
        page.flags = flags;
    }
}

void Memory::unmapPages(uint32_t vaddr, size_t size) {
    const size_t numPages = size >> PAGE_BITS;
    for (size_t i = 0; i < numPages; ++i) {
        auto& page = pageTable[(vaddr >> PAGE_BITS) + i];
        page.ptr = nullptr;
        page.flags = PageFlags::Unmapped;
    }
}

uint8_t* Memory::getPointer(uint32_t vaddr) {
    const auto& page = getPage(vaddr);
    if (!page.ptr) return nullptr;
    return page.ptr + (vaddr & (PAGE_SIZE - 1));
}

const uint8_t* Memory::getPointer(uint32_t vaddr) const {
    const auto& page = getPage(vaddr);
    if (!page.ptr) return nullptr;
    return page.ptr + (vaddr & (PAGE_SIZE - 1));
}

uint8_t Memory::read8(uint32_t vaddr) {
    auto* ptr = getPointer(vaddr);
    return ptr ? *ptr : 0;
}

uint16_t Memory::read16(uint32_t vaddr) {
    auto* ptr = getPointer(vaddr);
    if (!ptr) return 0;
    uint16_t val;
    std::memcpy(&val, ptr, sizeof(val));
    return val;
}

uint32_t Memory::read32(uint32_t vaddr) {
    auto* ptr = getPointer(vaddr);
    if (!ptr) return 0;
    uint32_t val;
    std::memcpy(&val, ptr, sizeof(val));
    return val;
}

uint64_t Memory::read64(uint32_t vaddr) {
    auto* ptr = getPointer(vaddr);
    if (!ptr) return 0;
    uint64_t val;
    std::memcpy(&val, ptr, sizeof(val));
    return val;
}

void Memory::write8(uint32_t vaddr, uint8_t value) {
    auto* ptr = getPointer(vaddr);
    if (ptr) *ptr = value;
}

void Memory::write16(uint32_t vaddr, uint16_t value) {
    auto* ptr = getPointer(vaddr);
    if (ptr) std::memcpy(ptr, &value, sizeof(value));
}

void Memory::write32(uint32_t vaddr, uint32_t value) {
    auto* ptr = getPointer(vaddr);
    if (ptr) std::memcpy(ptr, &value, sizeof(value));
}

void Memory::write64(uint32_t vaddr, uint64_t value) {
    auto* ptr = getPointer(vaddr);
    if (ptr) std::memcpy(ptr, &value, sizeof(value));
}

void Memory::readBlock(uint32_t vaddr, void* dest, size_t size) {
    auto* out = static_cast<uint8_t*>(dest);
    while (size > 0) {
        size_t pageOffset = vaddr & (PAGE_SIZE - 1);
        size_t chunk = std::min(size, PAGE_SIZE - pageOffset);
        auto* ptr = getPointer(vaddr);
        if (ptr) {
            std::memcpy(out, ptr, chunk);
        } else {
            std::memset(out, 0, chunk);
        }
        out += chunk;
        vaddr += static_cast<uint32_t>(chunk);
        size -= chunk;
    }
}

void Memory::writeBlock(uint32_t vaddr, const void* src, size_t size) {
    auto* in = static_cast<const uint8_t*>(src);
    while (size > 0) {
        size_t pageOffset = vaddr & (PAGE_SIZE - 1);
        size_t chunk = std::min(size, PAGE_SIZE - pageOffset);
        auto* ptr = getPointer(vaddr);
        if (ptr) {
            std::memcpy(ptr, in, chunk);
        }
        in += chunk;
        vaddr += static_cast<uint32_t>(chunk);
        size -= chunk;
    }
}

std::vector<Memory::MemoryRegion> Memory::getContiguousMappedRegions() const {
    std::vector<MemoryRegion> regions;

    uint32_t regionStart = 0;
    uint8_t* regionPtr = nullptr;
    size_t regionLen = 0;
    bool inRegion = false;

    for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i) {
        const auto& page = pageTable[i];
        uint32_t addr = static_cast<uint32_t>(i << PAGE_BITS);

        if (page.ptr) {
            if (inRegion && page.ptr == regionPtr + regionLen) {
                // Contiguous with current region
                regionLen += PAGE_SIZE;
            } else {
                // New region — save previous if exists
                if (inRegion) {
                    regions.push_back({regionStart, regionLen, const_cast<uint8_t*>(regionPtr)});
                }
                regionStart = addr;
                regionPtr = page.ptr;
                regionLen = PAGE_SIZE;
                inRegion = true;
            }
        } else {
            if (inRegion) {
                regions.push_back({regionStart, regionLen, const_cast<uint8_t*>(regionPtr)});
                inRegion = false;
            }
        }
    }

    if (inRegion) {
        regions.push_back({regionStart, regionLen, const_cast<uint8_t*>(regionPtr)});
    }

    return regions;
}

} // namespace Trident
