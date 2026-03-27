cmake_minimum_required(VERSION 3.20)
project(CrimsonDesertStorageCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# Dependencies
add_subdirectory(extern)

# Main DLL
add_library(StorageCraft SHARED
    src/main.cpp
    src/core/hook_manager.cpp
    src/core/memory.cpp
    src/core/logger.cpp
    src/game/inventory_accessor.cpp
    src/game/storage_accessor.cpp
    src/game/material_pool.cpp
    src/craft/craft_hook.cpp
    src/craft/material_consumer.cpp
    src/ui/ui_hook.cpp
    src/ui/storage_indicator.cpp
    src/config/mod_config.cpp
)

target_include_directories(StorageCraft PRIVATE src)
target_link_libraries(StorageCraft PRIVATE minhook nlohmann_json::nlohmann_json)
target_precompile_headers(StorageCraft PRIVATE src/pch.h)

set_target_properties(StorageCraft PROPERTIES
    OUTPUT_NAME "StorageCraft"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Unit tests (logic only, no hooking)
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(test)
endif()
