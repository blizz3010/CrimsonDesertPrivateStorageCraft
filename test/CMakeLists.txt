include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(StorageCraftTests
    test_material_pool.cpp
    test_material_consumer.cpp
    mock_inventory.h
)

target_include_directories(StorageCraftTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StorageCraftTests PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(StorageCraftTests)
