#include <catch2/catch_test_macros.hpp>

#include "loader/loader.hpp"

#include <cstdint>
#include <vector>

using namespace Trident;

// detectFormat requires at least 0x200 bytes before returning anything other than Unknown.
// Helper: build a zeroed 0x200-byte buffer with specific bytes set.
static std::vector<uint8_t> makeBuffer(std::initializer_list<std::pair<size_t, uint8_t>> patches) {
    std::vector<uint8_t> buf(0x200, 0);
    for (auto [offset, value] : patches) {
        buf[offset] = value;
    }
    return buf;
}

TEST_CASE("Loader: detect 3DSX format", "[loader]") {
    auto data = makeBuffer({{0,'3'},{1,'D'},{2,'S'},{3,'X'}});
    REQUIRE(Loader::detectFormat(data) == Loader::Format::ThreeDSX);
}

TEST_CASE("Loader: detect NCSD (CCI) format", "[loader]") {
    auto data = makeBuffer({{0x100,'N'},{0x101,'C'},{0x102,'S'},{0x103,'D'}});
    REQUIRE(Loader::detectFormat(data) == Loader::Format::CCI);
}

TEST_CASE("Loader: detect NCCH format", "[loader]") {
    auto data = makeBuffer({{0x100,'N'},{0x101,'C'},{0x102,'C'},{0x103,'H'}});
    REQUIRE(Loader::detectFormat(data) == Loader::Format::NCCH);
}

TEST_CASE("Loader: detect ELF format", "[loader]") {
    auto data = makeBuffer({{0,0x7F},{1,'E'},{2,'L'},{3,'F'}});
    REQUIRE(Loader::detectFormat(data) == Loader::Format::ELF);
}

TEST_CASE("Loader: all-zero buffer returns Unknown", "[loader]") {
    std::vector<uint8_t> data(0x200, 0);
    REQUIRE(Loader::detectFormat(data) == Loader::Format::Unknown);
}

TEST_CASE("Loader: buffer too small returns Unknown", "[loader]") {
    std::vector<uint8_t> small(0x10, 0);
    small[0] = '3'; small[1] = 'D'; small[2] = 'S'; small[3] = 'X';
    REQUIRE(Loader::detectFormat(small) == Loader::Format::Unknown);
}
