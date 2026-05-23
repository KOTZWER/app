cmake_minimum_required(VERSION 3.20)
project(TrayApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WIN32_EXECUTABLE ON)

# ---------------------------------------------------------------------------
# MIDL: generate RPC client stub (shared IDL from svc/)
# ---------------------------------------------------------------------------
find_program(MIDL_EXE midl HINTS
    "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/x64"
)

set(IDL_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/../svc/src/TrayService.idl")
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
        /client stub /server stub
        "${IDL_FILE}"
    DEPENDS "${IDL_FILE}"
    COMMENT "Running MIDL for TrayApp (client stub)"
)

add_custom_target(midl_gen_app DEPENDS
    "${IDL_OUT}/TrayService_h.h"
    "${IDL_OUT}/TrayService_c.c"
)

# ---------------------------------------------------------------------------
# TrayApp executable
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/app.rc
    "${IDL_OUT}/TrayService_c.c"   # client stub
)

add_executable(TrayApp WIN32 ${SOURCES})
add_dependencies(TrayApp midl_gen_app)

target_compile_definitions(TrayApp PRIVATE UNICODE _UNICODE)

target_include_directories(TrayApp PRIVATE
    src
    "${IDL_OUT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../svc/src"   # for rpc_client.h
)

target_link_libraries(TrayApp PRIVATE
    user32
    shell32
    comctl32
    advapi32
    rpcrt4
)

if(MSVC)
    target_compile_options(TrayApp PRIVATE /W4 /WX- /utf-8)
    set_target_properties(TrayApp PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\""
    )
endif()

install(TARGETS TrayApp RUNTIME DESTINATION bin)
