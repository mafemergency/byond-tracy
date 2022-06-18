cmake_minimum_required(
    VERSION 3.1
)

project(prof
    VERSION 0.1.0
    LANGUAGES C CXX
)

add_library(prof SHARED
    src/prof.c
    src/MinHook/buffer.c
    src/MinHook/hook.c
    src/MinHook/trampoline.c
    src/MinHook/hde/hde32.c
    src/tracy/TracyClient.cpp
)

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11 /W3 /WX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14 /W3 /WX")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /largeaddressaware")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -Wextra -pedantic -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -Wall -Wextra -pedantic -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-language-extension-token")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,/largeaddressaware")
endif()

# pass options along for tinkering
# possibly won't build with slimmed down in-tree tracy
macro(set_option option help value)
    option(${option} ${help} ${value})
    if(${option})
        message(STATUS "${option}: ON")
        target_compile_definitions(prof PUBLIC ${option})
    else()
        message(STATUS "${option}: OFF")
    endif()
endmacro()

set_option(TRACY_ENABLE "Enable profiling" ON)
set_option(TRACY_ON_DEMAND "On-demand profiling" OFF)
set_option(TRACY_CALLSTACK "Enfore callstack collection for tracy regions" OFF)
set_option(TRACY_NO_CALLSTACK "Disable all callstack related functionality" ON)
set_option(TRACY_NO_CALLSTACK_INLINES "Disables the inline functions in callstacks" ON)
set_option(TRACY_ONLY_LOCALHOST "Only listen on the localhost interface" ON)
set_option(TRACY_NO_BROADCAST "Disable client discovery by broadcast to local network" OFF)
set_option(TRACY_ONLY_IPV4 "Tracy will only accept connections on IPv4 addresses (disable IPv6)" OFF)
set_option(TRACY_NO_CODE_TRANSFER "Disable collection of source code" ON)
set_option(TRACY_NO_CONTEXT_SWITCH "Disable capture of context switches" ON)
set_option(TRACY_NO_EXIT "Client executable does not exit until all profile data is sent to server" ON)
set_option(TRACY_NO_FRAME_IMAGE "Disable capture of frame images" ON)
set_option(TRACY_NO_SAMPLING "Disable call stack sampling" ON)
set_option(TRACY_NO_VERIFY "Disable zone validation for C API" ON)
set_option(TRACY_NO_VSYNC_CAPTURE "Disable capture of hardware Vsync events" ON)
set_option(TRACY_NO_FRAME_IMAGE  "Disable the frame image support and its thread" ON)
set_option(TRACY_NO_SYS_TRACE  "Disable systrace sampling" ON)
set_option(TRACY_DELAYED_INIT "Enable delayed initialization of the library (init on first call)" OFF)
set_option(TRACY_MANUAL_LIFETIME "Enable the manual lifetime management of the profile" OFF)
set_option(TRACY_FIBERS "Enable fibers support" OFF)
