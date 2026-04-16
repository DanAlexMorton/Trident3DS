#include <catch2/catch_test_macros.hpp>

#include "memory/memory.hpp"

#include <cstdint>
#include <memory>

using namespace Trident;

TEST_CASE("Memory: init succeeds", "[memory]") {
    auto mem = std::make_unique<Memory>();
    REQUIRE(mem->init() == true);
}

TEST_CASE("Memory: unmapped reads return zero", "[memory]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    REQUIRE(mem->read8(VADDR_CODE_START)  == 0);
    REQUIRE(mem->read16(VADDR_CODE_START) == 0);
    REQUIRE(mem->read32(VADDR_CODE_START) == 0);
    REQUIRE(mem->read64(VADDR_CODE_START) == 0);
}

TEST_CASE("Memory: read/write round-trip 8-bit", "[memory]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    mem->mapPages(VADDR_CODE_START, mem->getFCRAM(), PAGE_SIZE, PageFlags::ReadWrite);

    mem->write8(VADDR_CODE_START, 0xAB);
    REQUIRE(mem->read8(VADDR_CODE_START) == 0xAB);
}

TEST_CASE("Memory: read/write round-trip 16-bit", "[memory]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    mem->mapPages(VADDR_CODE_START, mem->getFCRAM(), PAGE_SIZE, PageFlags::ReadWrite);

    mem->write16(VADDR_CODE_START, 0xDEAD);
    REQUIRE(mem->read16(VADDR_CODE_START) == 0xDEAD);
}

TEST_CASE("Memory: read/write round-trip 32-bit", "[memory]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    mem->mapPages(VADDR_CODE_START, mem->getFCRAM(), PAGE_SIZE, PageFlags::ReadWrite);

    mem->write32(VADDR_CODE_START, 0xDEADBEEF);
    REQUIRE(mem->read32(VADDR_CODE_START) == 0xDEADBEEF);
}

TEST_CASE("Memory: read/write round-trip 64-bit", "[memory]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    mem->mapPages(VADDR_CODE_START, mem->getFCRAM(), PAGE_SIZE, PageFlags::ReadWrite);

    mem->write64(VADDR_CODE_START, UINT64_C(0xCAFEBABEDEADBEEF));
    REQUIRE(mem->read64(VADDR_CODE_START) == UINT64_C(0xCAFEBABEDEADBEEF));
}
