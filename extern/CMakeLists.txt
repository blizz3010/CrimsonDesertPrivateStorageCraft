include(FetchContent)

# MinHook - x64 function hooking library
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageworthy/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# nlohmann/json - JSON parsing
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)
