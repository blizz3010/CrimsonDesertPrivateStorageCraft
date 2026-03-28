include(FetchContent)

# SafetyHook - modern x86_64 hooking library (used by CD modding community)
# Supports mid-function hooks, thread-safe, handles IP relocation via Zydis
FetchContent_Declare(
    safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(safetyhook)
