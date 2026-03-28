cmake_minimum_required(VERSION 3.16)

project(Simuless C)

# =========================
# Global configuration
# =========================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# =========================
# Compiler warnings
# =========================
add_compile_options(
    -Wall
    -Wextra
    -Werror
)

# =========================
# Options
# =========================
option(SIMULESS_BUILD_EXAMPLES "Build example programs" ON)

# =========================
# Modules
# =========================
add_subdirectory(telemetry)

# =========================
# Examples
# =========================
if(SIMULESS_BUILD_EXAMPLES)
    add_executable(telemetry_test
        examples/telemetry_test.c
    )

    target_link_libraries(telemetry_test telemetry)

    target_include_directories(telemetry_test PRIVATE
        telemetry/inc
    )
endif()