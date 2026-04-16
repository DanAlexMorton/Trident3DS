#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Trident {

// NCCH/NCSD/CXI ROM loader
struct NCCHHeader {
    uint32_t contentSize;
    uint64_t programId;
    uint32_t exefsOffset;
    uint32_t exefsSize;
    uint32_t romfsOffset;
    uint32_t romfsSize;
    uint32_t textCodeOffset;
    uint32_t textCodeSize;
    uint32_t roOffset;
    uint32_t roSize;
    uint32_t dataOffset;
    uint32_t dataSize;
    uint32_t bssSize;
};

class Loader {
public:
    enum class Format {
        Unknown,
        CCI,   // .3ds, .cci — NCSD container
        CXI,   // .cxi — single NCCH partition
        NCCH,  // .ncch, .app — NCCH
        ThreeDSX, // .3dsx — homebrew
        ELF,   // .elf, .axf
    };

    struct LoadResult {
        bool success = false;
        uint32_t entryPoint = 0;
        uint64_t programId = 0;
        std::string errorMessage;
    };

    static Format detectFormat(const std::vector<uint8_t>& data);
    static Format detectFormat(const std::string& path);

    // Load ROM into memory, returns entry point info
    static LoadResult load(const std::vector<uint8_t>& data, class Memory& memory);
    static LoadResult loadFile(const std::string& path, class Memory& memory);

private:
    static LoadResult loadNCSD(const std::vector<uint8_t>& data, class Memory& memory);
    static LoadResult loadNCCH(const std::vector<uint8_t>& data, class Memory& memory);
    static LoadResult load3DSX(const std::vector<uint8_t>& data, class Memory& memory);
    static LoadResult loadELF(const std::vector<uint8_t>& data, class Memory& memory);
};

} // namespace Trident
