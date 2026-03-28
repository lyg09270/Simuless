cmake_minimum_required(VERSION 3.16)

project(Simuless C)

# =========================
# Global configuration
# =========================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands (for clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# =========================
# Compiler warnings (global)
# =========================
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Werror
    )
elseif (MSVC)
    add_compile_options(
        /W4
        /WX
    )
endif()

# =========================
# Options
# =========================
option(SIMULESS_BUILD_EXAMPLES "Build example programs" ON)

# =========================
# Modules
# =========================
add_subdirectory(telemetry)

# 未来扩展（先留）
# add_subdirectory(core)
# add_subdirectory(transport)

# =========================
# Examples
# =========================
if(SIMULESS_BUILD_EXAMPLES)

    file(GLOB EXAMPLE_SRC
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.c
    )

    foreach(example_file ${EXAMPLE_SRC})

        get_filename_component(example_name ${example_file} NAME_WE)

        add_executable(${example_name} ${example_file})

        target_link_libraries(${example_name}
            telemetry
        )

        target_include_directories(${example_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/telemetry/inc
        )

    endforeach()

endif()