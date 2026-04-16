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
    if (data.size() < 0x200) {
        return {false, 0, 0, "File too small for NCCH"};
    }

    // Verify NCCH magic at offset 0x100 (0x000–0x0FF is RSA-2048 signature)
    if (data[0x100] != 'N' || data[0x101] != 'C' ||
        data[0x102] != 'C' || data[0x103] != 'H') {
        return {false, 0, 0, "Invalid NCCH magic"};
    }

    uint64_t programId = 0;
    std::memcpy(&programId, data.data() + 0x118, 8);

    // ExeFS offset is at NCCH+0x180 (in media units = 0x200 bytes each)
    // Note: 0x1A0 in the brief is incorrect — that offset holds RomFS size.
    uint32_t exefsOffsetUnits = 0;
    std::memcpy(&exefsOffsetUnits, data.data() + 0x180, 4);
    const uint32_t exefsOffset = exefsOffsetUnits * 0x200;

    // ExeFS header is 0x200 bytes: 8 × 16-byte file entries, then hash table
    if (exefsOffset + 0x200 > data.size()) {
        return {false, 0, 0, "NCCH: ExeFS header out of bounds"};
    }

    // Find ".code" in ExeFS file entry table (8 entries, each 16 bytes)
    const uint8_t* exefsHeader = data.data() + exefsOffset;
    uint32_t codeFileOffset = 0;
    uint32_t codeFileSize   = 0;
    bool     foundCode      = false;

    for (int entry = 0; entry < 8; entry++) {
        const uint8_t* fe = exefsHeader + entry * 16;
        if (std::memcmp(fe, ".code\0\0\0", 8) == 0) {
            std::memcpy(&codeFileOffset, fe + 8,  4);
            std::memcpy(&codeFileSize,   fe + 12, 4);
            foundCode = true;
            break;
        }
    }

    if (!foundCode || codeFileSize == 0) {
        return {false, 0, 0, "NCCH: .code entry not found in ExeFS"};
    }

    // .code data: ExeFS offset + 0x200 (entries) + file's own offset
    const uint32_t codeDataOffset = exefsOffset + 0x200 + codeFileOffset;
    if ((size_t)codeDataOffset + codeFileSize > data.size()) {
        return {false, 0, 0, "NCCH: .code section out of bounds"};
    }

    // TODO(Phase 2): add LZ11 decompression for compressed .code sections

    // Map .code at VADDR_CODE_START, backed by FCRAM starting at offset 0
    auto pageAlignSize = [](uint32_t sz) -> uint32_t {
        return (sz + static_cast<uint32_t>(PAGE_SIZE) - 1u) &
               ~(static_cast<uint32_t>(PAGE_SIZE) - 1u);
    };

    uint8_t* fcram = memory.getFCRAM();
    memory.mapPages(VADDR_CODE_START, fcram, pageAlignSize(codeFileSize), PageFlags::All);
    memory.writeBlock(VADDR_CODE_START, data.data() + codeDataOffset, codeFileSize);

    fprintf(stderr, "[LOADER] NCCH: loaded .code (%u bytes) at 0x%08X, programId=0x%016llX\n",
            codeFileSize, VADDR_CODE_START,
            static_cast<unsigned long long>(programId));

    return {true, VADDR_CODE_START, programId, ""};
}

Loader::LoadResult Loader::load3DSX(const std::vector<uint8_t>& data, Memory& memory) {
    // Minimum: 0x20-byte header + 3 × 0x0C relocation headers = 0x44 bytes
    if (data.size() < 0x44) {
        return {false, 0, 0, "3DSX file too small"};
    }

    // Validate magic (already checked in detectFormat, but be explicit)
    if (data[0] != '3' || data[1] != 'D' || data[2] != 'S' || data[3] != 'X') {
        return {false, 0, 0, "3DSX: invalid magic"};
    }

    // ---------------------------------------------------------------
    // Parse main header (all fields little-endian)
    // ---------------------------------------------------------------
    uint16_t headerSize = 0;
    std::memcpy(&headerSize, data.data() + 0x04, 2);

    uint32_t codeSize = 0, rodataSize = 0, bssSize = 0, dataSize = 0;
    std::memcpy(&codeSize,   data.data() + 0x10, 4);
    std::memcpy(&rodataSize, data.data() + 0x14, 4);
    std::memcpy(&bssSize,    data.data() + 0x18, 4);
    std::memcpy(&dataSize,   data.data() + 0x1C, 4); // includes BSS

    if (codeSize == 0) {
        return {false, 0, 0, "3DSX: code segment is empty"};
    }
    if (dataSize < bssSize) {
        return {false, 0, 0, "3DSX: bss_size exceeds data_size"};
    }

    // ---------------------------------------------------------------
    // Compute page-aligned segment load addresses
    // Convention: virtual 0x00100000 maps to FCRAM[0].
    // Each segment starts on a 4 KB page boundary.
    // ---------------------------------------------------------------
    auto pageAlignAddr = [](uint32_t addr) -> uint32_t {
        return (addr + static_cast<uint32_t>(PAGE_SIZE) - 1u) &
               ~(static_cast<uint32_t>(PAGE_SIZE) - 1u);
    };
    auto pageAlignSize = [](uint32_t sz) -> uint32_t {
        return (sz + static_cast<uint32_t>(PAGE_SIZE) - 1u) &
               ~(static_cast<uint32_t>(PAGE_SIZE) - 1u);
    };

    const uint32_t codeBase   = VADDR_CODE_START;                       // 0x00100000
    const uint32_t rodataBase = pageAlignAddr(codeBase + codeSize);
    const uint32_t dataBase   = pageAlignAddr(rodataBase + rodataSize);

    // ---------------------------------------------------------------
    // Three relocation headers follow the main header (one per segment)
    // Each is 12 bytes: numAbs (u32), numRel (u32), reserved (u32)
    // ---------------------------------------------------------------
    struct RelocHeader {
        uint32_t numAbs;
        uint32_t numRel;
        uint32_t reserved;
    };
    static_assert(sizeof(RelocHeader) == 0x0C, "RelocHeader must be 12 bytes");

    if (headerSize < 0x20 ||
        static_cast<size_t>(headerSize) + 3u * sizeof(RelocHeader) > data.size()) {
        return {false, 0, 0, "3DSX: relocation headers out of bounds"};
    }

    RelocHeader relocHdr[3];
    for (int i = 0; i < 3; i++) {
        std::memcpy(&relocHdr[i],
                    data.data() + headerSize + static_cast<size_t>(i) * sizeof(RelocHeader),
                    sizeof(RelocHeader));
    }

    // ---------------------------------------------------------------
    // Locate segments in file:
    //   file layout: [main header] [3 reloc headers] [code] [rodata] [data±bss]
    // On disk, data segment stores only (dataSize - bssSize) bytes;
    // the BSS tail is zero-filled by FCRAM initialisation.
    // ---------------------------------------------------------------
    const size_t   segOffset    = static_cast<size_t>(headerSize) + 3u * sizeof(RelocHeader);
    const uint32_t dataDiskSize = dataSize - bssSize;   // bytes actually in file
    const size_t   totalSegFile = static_cast<size_t>(codeSize) + rodataSize + dataDiskSize;

    if (segOffset + totalSegFile > data.size()) {
        return {false, 0, 0, "3DSX: segment data extends beyond file"};
    }

    // Sanity-check against FCRAM capacity
    const uint32_t totalVirtSize = (dataBase - codeBase) + dataSize;
    if (totalVirtSize > memory.getFCRAMSize()) {
        return {false, 0, 0, "3DSX: segments exceed FCRAM capacity"};
    }

    // ---------------------------------------------------------------
    // Map virtual address ranges into FCRAM and copy segment bytes
    // Virtual 0x00100000 → FCRAM[0], so fcram_offset = vaddr - codeBase
    // ---------------------------------------------------------------
    uint8_t* const fcram = memory.getFCRAM();

    memory.mapPages(codeBase, fcram, pageAlignSize(codeSize), PageFlags::All);
    memory.writeBlock(codeBase, data.data() + segOffset, codeSize);

    if (rodataSize > 0) {
        memory.mapPages(rodataBase,
                        fcram + (rodataBase - codeBase),
                        pageAlignSize(rodataSize),
                        PageFlags::ReadWrite);
        memory.writeBlock(rodataBase,
                          data.data() + segOffset + codeSize,
                          rodataSize);
    }

    if (dataSize > 0) {
        memory.mapPages(dataBase,
                        fcram + (dataBase - codeBase),
                        pageAlignSize(dataSize),
                        PageFlags::ReadWrite);
        if (dataDiskSize > 0) {
            memory.writeBlock(dataBase,
                              data.data() + segOffset + codeSize + rodataSize,
                              dataDiskSize);
        }
        // BSS tail (dataBase + dataDiskSize .. dataDiskSize + bssSize) is
        // already zero because FCRAM is zero-initialised in Memory::init().
    }

    fprintf(stderr, "[LOADER] 3DSX: code=0x%08X (%u B), rodata=0x%08X (%u B), "
                    "data=0x%08X (%u B, bss=%u B)\n",
            codeBase, codeSize, rodataBase, rodataSize, dataBase, dataSize, bssSize);

    // ---------------------------------------------------------------
    // Apply relocation tables
    // Two tables per segment (ABS then REL), three segments → six total.
    // Tables appear in the file AFTER all segment bytes.
    //
    // Each relocation entry (4 bytes):
    //   bits 31–16  skip count  – number of words to advance without patching
    //   bits 15–0   patch count – number of words to patch after the skip
    //
    // Word encoding (the value already in memory at the patch site):
    //   bits 31–28  target segment index (0=code, 1=rodata, 2=data)
    //   bits 27–0   byte offset within that segment
    //
    // ABS patch:  write32(addr, segBase[tSeg] + tOfs)
    // REL patch:  write32(addr, (segBase[tSeg] + tOfs) - addr)
    // ---------------------------------------------------------------
    const uint32_t segBase[3] = { codeBase, rodataBase, dataBase };
    size_t relocOffset = segOffset + totalSegFile;

    for (int seg = 0; seg < 3; seg++) {
        // --- ABS table ---
        uint32_t curWord = segBase[seg];
        for (uint32_t i = 0; i < relocHdr[seg].numAbs; i++) {
            if (relocOffset + 4u > data.size()) {
                fprintf(stderr, "[LOADER] 3DSX: ABS reloc table truncated (seg %d, entry %u)\n",
                        seg, i);
                break;
            }
            uint32_t entry = 0;
            std::memcpy(&entry, data.data() + relocOffset, 4);
            relocOffset += 4;

            const uint32_t skipWords  = entry >> 16;
            const uint32_t patchWords = entry & 0xFFFFu;

            curWord += skipWords * 4u;
            for (uint32_t j = 0; j < patchWords; j++) {
                const uint32_t word = memory.read32(curWord);
                const uint32_t tSeg = word >> 28;
                const uint32_t tOfs = word & 0x0FFFFFFFu;
                if (tSeg < 3u) {
                    memory.write32(curWord, segBase[tSeg] + tOfs);
                } else {
                    fprintf(stderr, "[LOADER] 3DSX: ABS bad seg %u at 0x%08X\n", tSeg, curWord);
                }
                curWord += 4u;
            }
        }

        // --- REL table ---
        curWord = segBase[seg];
        for (uint32_t i = 0; i < relocHdr[seg].numRel; i++) {
            if (relocOffset + 4u > data.size()) {
                fprintf(stderr, "[LOADER] 3DSX: REL reloc table truncated (seg %d, entry %u)\n",
                        seg, i);
                break;
            }
            uint32_t entry = 0;
            std::memcpy(&entry, data.data() + relocOffset, 4);
            relocOffset += 4;

            const uint32_t skipWords  = entry >> 16;
            const uint32_t patchWords = entry & 0xFFFFu;

            curWord += skipWords * 4u;
            for (uint32_t j = 0; j < patchWords; j++) {
                const uint32_t word = memory.read32(curWord);
                const uint32_t tSeg = word >> 28;
                const uint32_t tOfs = word & 0x0FFFFFFFu;
                if (tSeg < 3u) {
                    memory.write32(curWord, (segBase[tSeg] + tOfs) - curWord);
                } else {
                    fprintf(stderr, "[LOADER] 3DSX: REL bad seg %u at 0x%08X\n", tSeg, curWord);
                }
                curWord += 4u;
            }
        }
    }

    fprintf(stderr, "[LOADER] 3DSX: load complete, entry=0x%08X\n", codeBase);
    return {true, codeBase, 0, ""};
}

Loader::LoadResult Loader::loadELF(const std::vector<uint8_t>& data, Memory& memory) {
    // TODO: Parse ELF header and load segments
    (void)memory;
    return {false, 0, 0, "ELF loading not yet implemented"};
}

} // namespace Trident
