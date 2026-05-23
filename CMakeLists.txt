cmake_minimum_required(VERSION 3.20)
project(TrayApp VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Win32 subsystem (no console window)
# ---------------------------------------------------------------------------
set(CMAKE_WIN32_EXECUTABLE ON)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
)

# Resource file (only included on Windows)
if(WIN32)
    list(APPEND SOURCES src/app.rc)
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(TrayApp WIN32 ${SOURCES})

# ---------------------------------------------------------------------------
# Include dirs
# ---------------------------------------------------------------------------
target_include_directories(TrayApp PRIVATE src)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_compile_definitions(TrayApp PRIVATE UNICODE _UNICODE)

target_link_libraries(TrayApp PRIVATE
    user32
    shell32
    comctl32
)

# ---------------------------------------------------------------------------
# Compiler options (MSVC)
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(TrayApp PRIVATE /W4 /WX- /utf-8)
    # Embed manifest so UAC / DPI settings work correctly
    set_target_properties(TrayApp PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\""
    )
endif()

# ---------------------------------------------------------------------------
# Install / artifact staging
# ---------------------------------------------------------------------------
install(TARGETS TrayApp
    RUNTIME DESTINATION bin
)

# ---------------------------------------------------------------------------
# Copy icon to binary directory (so the .rc lookup path works at build time)
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/app.ico")
    file(COPY "${CMAKE_SOURCE_DIR}/resources/app.ico"
         DESTINATION "${CMAKE_BINARY_DIR}/resources")
    # Also copy next to the .rc so the resource compiler finds it
    configure_file(
        "${CMAKE_SOURCE_DIR}/resources/app.ico"
        "${CMAKE_SOURCE_DIR}/src/../resources/app.ico"
        COPYONLY
    )
endif()
