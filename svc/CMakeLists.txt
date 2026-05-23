cmake_minimum_required(VERSION 3.20)
project(TrayService VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# MIDL compilation: TrayService.idl → TrayService_h.h + TrayService_c.c + TrayService_s.c
# ---------------------------------------------------------------------------
find_program(MIDL_EXE midl HINTS
    "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/x64"
)

set(IDL_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/src/TrayService.idl")
set(IDL_OUT   "${CMAKE_CURRENT_BINARY_DIR}/midl")

file(MAKE_DIRECTORY "${IDL_OUT}")

add_custom_command(
    OUTPUT
        "${IDL_OUT}/TrayService_h.h"
        "${IDL_OUT}/TrayService_c.c"
        "${IDL_OUT}/TrayService_s.c"
    COMMAND "${MIDL_EXE}"
        /nologo
        /out "${IDL_OUT}"
        /header TrayService_h.h
        /client stub  /server stub
        "${IDL_FILE}"
    DEPENDS "${IDL_FILE}"
    COMMENT "Running MIDL on TrayService.idl"
)

add_custom_target(midl_gen DEPENDS
    "${IDL_OUT}/TrayService_h.h"
    "${IDL_OUT}/TrayService_c.c"
    "${IDL_OUT}/TrayService_s.c"
)

# ---------------------------------------------------------------------------
# TrayService executable
# ---------------------------------------------------------------------------
add_executable(TrayService
    src/service.cpp
    "${IDL_OUT}/TrayService_s.c"   # server stub
)

add_dependencies(TrayService midl_gen)

target_compile_definitions(TrayService PRIVATE UNICODE _UNICODE)

target_include_directories(TrayService PRIVATE
    src
    "${IDL_OUT}"
)

target_link_libraries(TrayService PRIVATE
    advapi32
    wtsapi32
    userenv
    rpcrt4
)

if(MSVC)
    target_compile_options(TrayService PRIVATE /W4 /WX- /utf-8)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS TrayService RUNTIME DESTINATION bin)
