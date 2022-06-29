cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_STANDARD 11)

project(prof
    VERSION 0.1.0
    LANGUAGES C
)

add_library(prof SHARED
    src/prof.c
    src/microtracy/lz4.c
    src/microtracy/queue.c
    src/microtracy/server.c
    src/microtracy/microtracy.c
    src/MinHook/buffer.c
    src/MinHook/hook.c
    src/MinHook/trampoline.c
    src/MinHook/hde/hde32.c
)

target_link_libraries(prof
    Ws2_32.lib
)

if(MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -D_WIN32_WINNT=0x600")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--large-address-aware")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,/largeaddressaware")
endif()
