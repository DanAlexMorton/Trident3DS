#include "patcher.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace Trident {

static uint32_t crc32Table[256];
static bool crc32Initialized = false;

static void initCRC32() {
    if (crc32Initialized) return;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        crc32Table[i] = c;
    }
    crc32Initialized = true;
}

uint32_t Patcher::crc32(const uint8_t* data, size_t size) {
    initCRC32();
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i) {
        crc = crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

PatchFormat Patcher::detect(const std::vector<uint8_t>& patch) {
    if (patch.size() >= 5 && patch[0] == 'P' && patch[1] == 'A' &&
        patch[2] == 'T' && patch[3] == 'C' && patch[4] == 'H') {
        return PatchFormat::IPS;
    }
    if (patch.size() >= 4 && patch[0] == 'U' && patch[1] == 'P' &&
        patch[2] == 'S' && patch[3] == '1') {
        return PatchFormat::UPS;
    }
    if (patch.size() >= 4 && patch[0] == 'B' && patch[1] == 'P' &&
        patch[2] == 'S' && patch[3] == '1') {
        return PatchFormat::BPS;
    }
    return PatchFormat::Unknown;
}

bool Patcher::apply(std::vector<uint8_t>& rom, const std::string& patchPath) {
    std::ifstream file(patchPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> patch(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(patch.data()), size);
    return apply(rom, patch);
}

bool Patcher::apply(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch) {
    switch (detect(patch)) {
        case PatchFormat::IPS: return applyIPS(rom, patch);
        case PatchFormat::UPS: return applyUPS(rom, patch);
        case PatchFormat::BPS: return applyBPS(rom, patch);
        default: return false;
    }
}

// IPS format: "PATCH" header, then records of [offset:3][size:2][data:size], EOF marker "EOF"
// RLE: [offset:3][0x0000][rle_size:2][rle_byte:1]
bool Patcher::applyIPS(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch) {
    if (patch.size() < 8) return false;

    size_t pos = 5; // Skip "PATCH" header

    while (pos + 3 <= patch.size()) {
        // Check for EOF marker
        if (patch[pos] == 'E' && patch[pos + 1] == 'O' && patch[pos + 2] == 'F') {
            // Optional truncation extension (3 bytes after EOF)
            if (pos + 6 <= patch.size()) {
                uint32_t truncSize = (patch[pos + 3] << 16) | (patch[pos + 4] << 8) | patch[pos + 5];
                rom.resize(truncSize);
            }
            return true;
        }

        // Read 3-byte big-endian offset
        uint32_t offset = (patch[pos] << 16) | (patch[pos + 1] << 8) | patch[pos + 2];
        pos += 3;

        if (pos + 2 > patch.size()) return false;

        // Read 2-byte big-endian size
        uint16_t size = (patch[pos] << 8) | patch[pos + 1];
        pos += 2;

        if (size == 0) {
            // RLE record
            if (pos + 3 > patch.size()) return false;
            uint16_t rleSize = (patch[pos] << 8) | patch[pos + 1];
            uint8_t rleByte = patch[pos + 2];
            pos += 3;

            if (offset + rleSize > rom.size()) {
                rom.resize(offset + rleSize);
            }
            std::memset(rom.data() + offset, rleByte, rleSize);
        } else {
            // Normal record
            if (pos + size > patch.size()) return false;

            if (offset + size > rom.size()) {
                rom.resize(offset + size);
            }
            std::memcpy(rom.data() + offset, patch.data() + pos, size);
            pos += size;
        }
    }

    return false; // No EOF marker found
}

// UPS format: "UPS1" header, varint source size, varint target size, then XOR diffs
bool Patcher::applyUPS(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch) {
    if (patch.size() < 18) return false; // Header + 3 CRC32s + minimum data

    const uint8_t* ptr = patch.data() + 4; // Skip "UPS1"
    const uint8_t* end = patch.data() + patch.size() - 12; // Last 12 bytes are CRC32s

    // Verify patch CRC32
    uint32_t patchCRC = crc32(patch.data(), patch.size() - 4);
    uint32_t storedPatchCRC;
    std::memcpy(&storedPatchCRC, patch.data() + patch.size() - 4, 4);
    if (patchCRC != storedPatchCRC) return false;

    // Read source and target sizes
    auto readVarint = [](const uint8_t*& p, const uint8_t* end) -> uint64_t {
        uint64_t value = 0;
        unsigned shift = 0;
        while (p < end) {
            uint8_t b = *p++;
            value += static_cast<uint64_t>(b & 0x7F) << shift;
            if (b & 0x80) return value;
            value += uint64_t(1) << shift;
            shift += 7;
        }
        return value;
    };

    uint64_t sourceSize = readVarint(ptr, end);
    uint64_t targetSize = readVarint(ptr, end);

    if (rom.size() != sourceSize) return false;
    rom.resize(static_cast<size_t>(targetSize));

    // Apply XOR diffs
    size_t offset = 0;
    while (ptr < end) {
        uint64_t skip = readVarint(ptr, end);
        offset += static_cast<size_t>(skip);

        while (ptr < end && *ptr != 0x00) {
            if (offset < rom.size()) {
                rom[offset] ^= *ptr;
            }
            ++ptr;
            ++offset;
        }
        if (ptr < end) ++ptr; // Skip terminator
        ++offset;
    }

    return true;
}

uint64_t Patcher::decodeBPS(const uint8_t*& ptr, const uint8_t* end) {
    uint64_t value = 0;
    unsigned shift = 0;
    while (ptr < end) {
        uint8_t b = *ptr++;
        value += static_cast<uint64_t>(b & 0x7F) << shift;
        if (b & 0x80) return value;
        value += uint64_t(1) << shift;
        shift += 7;
    }
    return 0;
}

// BPS format: "BPS1" header, variable-length encoding, source/target/metadata, actions, CRC32s
bool Patcher::applyBPS(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch) {
    if (patch.size() < 19) return false;

    // Verify patch CRC
    uint32_t patchCRC = crc32(patch.data(), patch.size() - 4);
    uint32_t storedPatchCRC;
    std::memcpy(&storedPatchCRC, patch.data() + patch.size() - 4, 4);
    if (patchCRC != storedPatchCRC) return false;

    const uint8_t* ptr = patch.data() + 4; // Skip "BPS1"
    const uint8_t* end = patch.data() + patch.size() - 12;

    uint64_t sourceSize = decodeBPS(ptr, end);
    uint64_t targetSize = decodeBPS(ptr, end);
    uint64_t metadataSize = decodeBPS(ptr, end);
    ptr += metadataSize; // Skip metadata

    if (rom.size() != sourceSize) return false;

    std::vector<uint8_t> source = rom;
    std::vector<uint8_t> target(static_cast<size_t>(targetSize), 0);

    size_t outputOffset = 0;
    int64_t sourceRelOffset = 0;
    int64_t targetRelOffset = 0;

    while (ptr < end && outputOffset < target.size()) {
        uint64_t cmd = decodeBPS(ptr, end);
        uint64_t length = (cmd >> 2) + 1;
        unsigned action = cmd & 3;

        switch (action) {
            case 0: // SourceRead
                for (uint64_t i = 0; i < length && outputOffset < target.size(); ++i) {
                    target[outputOffset] = (outputOffset < source.size()) ? source[outputOffset] : 0;
                    ++outputOffset;
                }
                break;

            case 1: // TargetRead
                for (uint64_t i = 0; i < length && outputOffset < target.size() && ptr < end; ++i) {
                    target[outputOffset++] = *ptr++;
                }
                break;

            case 2: { // SourceCopy
                uint64_t data = decodeBPS(ptr, end);
                sourceRelOffset += (data & 1) ? -(int64_t)(data >> 1) : (int64_t)(data >> 1);
                for (uint64_t i = 0; i < length && outputOffset < target.size(); ++i) {
                    target[outputOffset++] = (sourceRelOffset < (int64_t)source.size()) ? source[sourceRelOffset] : 0;
                    ++sourceRelOffset;
                }
                break;
            }

            case 3: { // TargetCopy
                uint64_t data = decodeBPS(ptr, end);
                targetRelOffset += (data & 1) ? -(int64_t)(data >> 1) : (int64_t)(data >> 1);
                for (uint64_t i = 0; i < length && outputOffset < target.size(); ++i) {
                    target[outputOffset++] = target[targetRelOffset];
                    ++targetRelOffset;
                }
                break;
            }
        }
    }

    rom = std::move(target);
    return true;
}

} // namespace Trident
