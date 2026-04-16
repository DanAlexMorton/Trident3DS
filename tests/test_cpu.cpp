#include <catch2/catch_test_macros.hpp>

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"

#include <memory>

using namespace Trident;

TEST_CASE("CPU: init succeeds", "[cpu]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    CPU cpu;
    REQUIRE(cpu.init(*mem) == true);
}

TEST_CASE("CPU: getPC returns 0 after reset", "[cpu]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    CPU cpu;
    cpu.init(*mem);
    cpu.reset();

    REQUIRE(cpu.getPC() == 0u);
}

TEST_CASE("CPU: setPC / getPC round-trip", "[cpu]") {
    auto mem = std::make_unique<Memory>();
    mem->init();

    CPU cpu;
    cpu.init(*mem);

    cpu.setPC(0x00100000u);
    REQUIRE(cpu.getPC() == 0x00100000u);
}
