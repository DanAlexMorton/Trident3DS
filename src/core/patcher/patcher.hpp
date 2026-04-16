#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Trident {

enum class PatchFormat {
    Unknown,
    IPS,
    UPS,
    BPS,
};

class Patcher {
public:
    // Auto-detect format and apply patch to ROM data in-place
    static bool apply(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch);
    static bool apply(std::vector<uint8_t>& rom, const std::string& patchPath);

    // Detect patch format from data
    static PatchFormat detect(const std::vector<uint8_t>& patch);

    // Format-specific apply
    static bool applyIPS(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch);
    static bool applyUPS(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch);
    static bool applyBPS(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch);

private:
    static uint32_t crc32(const uint8_t* data, size_t size);
    static uint64_t decodeBPS(const uint8_t*& ptr, const uint8_t* end);
};

} // namespace Trident
