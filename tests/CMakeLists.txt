include(CTest)
include(Catch)

add_executable(tests
    test_config.cpp
    # Phase 2+: add test_price_process.cpp, test_strategy.cpp, etc.
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

# Automatically register every TEST_CASE with CTest
catch_discover_tests(tests)
