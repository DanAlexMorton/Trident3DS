add_executable(trident_tests
    test_memory.cpp
    test_loader.cpp
    test_cpu.cpp
)

target_link_libraries(trident_tests PRIVATE
    trident_core
    Catch2::Catch2WithMain
)

target_include_directories(trident_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/include
)

include(Catch)
catch_discover_tests(trident_tests)
