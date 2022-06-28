cmake_minimum_required(
    VERSION 3.1
)

project(prof
    VERSION 0.1.0
    LANGUAGES C CXX
)

add_library(prof SHARED
    src/prof.c
    src/microtracy/lz4.c
    src/microtracy/queue.c
    src/microtracy/microtracy.c
    src/MinHook/buffer.c
    src/MinHook/hook.c
    src/MinHook/trampoline.c
    src/MinHook/hde/hde32.c
)

target_link_libraries(prof
    Ws2_32.lib
)

target_compile_definitions(prof PUBLIC
    USE_MICROTRACY
)

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11 /W3 /WX")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE /LTCG /INCREMENTAL:NO")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fansi-escape-codes -fcolor-diagnostics -m32 -Wall -Wextra -pedantic -Werror -flto=full")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,/largeaddressaware")
endif()
