#include "loader.hpp"
#include "../memory/memory.hpp"
#include <cstring>
#include <fstream>

namespace Trident {

Loader::Format Loader::detectFormat(const std::vector<uint8_t>& data) {
    if (data.size() < 0x200) return Format::Unknown;

    // NCSD: magic at offset 0x100
    if (data.size() >= 0x104 &&
        data[0x100] == 'N' && data[0x101] == 'C' &&
        data[0x102] == 'S' && data[0x103] == 'D') {
        return Format::CCI;
    }

    // NCCH: magic at offset 0x100
    if (data.size() >= 0x104 &&
        data[0x100] == 'N' && data[0x101] == 'C' &&
        data[0x102] == 'C' && data[0x103] == 'H') {
        return Format::NCCH;
    }

    // 3DSX: magic "3DSX" at offset 0
    if (data.size() >= 4 &&
        data[0] == '3' && data[1] == 'D' &&
        data[2] == 'S' && data[3] == 'X') {
        return Format::ThreeDSX;
    }

    // ELF: magic 0x7F "ELF"
    if (data.size() >= 4 &&
        data[0] == 0x7F && data[1] == 'E' &&
        data[2] == 'L' && data[3] == 'F') {
        return Format::ELF;
    }

    return Format::Unknown;
}

Loader::Format Loader::detectFormat(const std::string& path) {
    // Check extension first
    auto ext = path.substr(path.find_last_of('.') + 1);
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));

    if (ext == "cci" || ext == "3ds") return Format::CCI;
    if (ext == "cxi") return Format::CXI;
    if (ext == "ncch" || ext == "app") return Format::NCCH;
    if (ext == "3dsx") return Format::ThreeDSX;
    if (ext == "elf" || ext == "axf") return Format::ELF;

    return Format::Unknown;
}

Loader::LoadResult Loader::loadFile(const std::string& path, Memory& memory) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {false, 0, 0, "Failed to open file: " + path};
    }

    auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);

    return load(data, memory);
}

Loader::LoadResult Loader::load(const std::vector<uint8_t>& data, Memory& memory) {
    auto format = detectFormat(data);
    switch (format) {
        case Format::CCI:    return loadNCSD(data, memory);
        case Format::CXI:
        case Format::NCCH:   return loadNCCH(data, memory);
        case Format::ThreeDSX: return load3DSX(data, memory);
        case Format::ELF:    return loadELF(data, memory);
        default:
            return {false, 0, 0, "Unknown ROM format"};
    }
}

Loader::LoadResult Loader::loadNCSD(const std::vector<uint8_t>& data, Memory& memory) {
    // NCSD is a container with up to 8 NCCH partitions
    // Partition 0 is the main executable
    if (data.size() < 0x4000) {
        return {false, 0, 0, "File too small for NCSD"};
    }

    // Read partition table offset (at 0x120, 8 bytes per entry)
    uint32_t part0Offset = 0;
    uint32_t part0Size = 0;
    std::memcpy(&part0Offset, data.data() + 0x120, 4);
    std::memcpy(&part0Size, data.data() + 0x124, 4);

    // Offsets are in media units (0x200 bytes)
    part0Offset *= 0x200;
    part0Size *= 0x200;

    if (part0Offset + part0Size > data.size()) {
        return {false, 0, 0, "Partition 0 extends beyond file"};
    }

    // Extract partition 0 and load as NCCH
    std::vector<uint8_t> partition(data.begin() + part0Offset,
                                    data.begin() + part0Offset + part0Size);
    return loadNCCH(partition, memory);
}

Loader::LoadResult Loader::loadNCCH(const std::vector<uint8_t>& data, Memory& memory) {
    // TODO: Full NCCH parsing
    // - Read ExeFS (contains .code, .banner, .icon)
    // - Decompress .code section (LZSS)
    // - Load code into memory at VADDR_CODE_START
    // - Parse ExHeader for entry point, stack size, BSS

    if (data.size() < 0x200) {
        return {false, 0, 0, "File too small for NCCH"};
    }

    // Verify NCCH magic
    if (data[0x100] != 'N' || data[0x101] != 'C' ||
        data[0x102] != 'C' || data[0x103] != 'H') {
        return {false, 0, 0, "Invalid NCCH magic"};
    }

    uint64_t programId = 0;
    std::memcpy(&programId, data.data() + 0x118, 8);

    // Stub: return success with default entry point
    return {true, VADDR_CODE_START, programId, ""};
}

Loader::LoadResult Loader::load3DSX(const std::vector<uint8_t>& data, Memory& memory) {
    // TODO: Parse 3DSX header and load segments
    (void)memory;
    return {false, 0, 0, "3DSX loading not yet implemented"};
}

Loader::LoadResult Loader::loadELF(const std::vector<uint8_t>& data, Memory& memory) {
    // TODO: Parse ELF header and load segments
    (void)memory;
    return {false, 0, 0, "ELF loading not yet implemented"};
}

} // namespace Trident
