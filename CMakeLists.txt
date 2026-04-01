cmake_minimum_required(VERSION 3.16)

project(Simuless C)

function(simuless_add_module_if_exists dir)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
    endif()
endfunction()

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
option(SIMULESS_BUILD_TESTS "Build unit tests" ON)
option(SIMULESS_AUTO_RUN_TESTS_ON_BUILD "Run tests automatically during build" ON)

# =========================
# Modules
# =========================
simuless_add_module_if_exists(osal)
simuless_add_module_if_exists(telemetry)
simuless_add_module_if_exists(smls_io)
simuless_add_module_if_exists(transport)
# 未来扩展（先留）
# add_subdirectory(core)


# =========================
# Examples
# =========================
if(SIMULESS_BUILD_EXAMPLES)

    if(NOT TARGET telemetry OR NOT TARGET osal OR NOT TARGET transport)
        message(STATUS "Skip examples: required targets telemetry/osal/transport are not all available")
    else()

    file(GLOB EXAMPLE_SRC
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.c
    )

    foreach(example_file ${EXAMPLE_SRC})

        get_filename_component(example_name ${example_file} NAME_WE)

        add_executable(${example_name} ${example_file})

        target_link_libraries(${example_name}
            telemetry
            transport
            osal
        )

    endforeach()

    endif()

endif()

# =========================
# Tests
# =========================
if(SIMULESS_BUILD_TESTS)
    include(CTest)

    if(BUILD_TESTING)
        add_subdirectory(tests)

        if(SIMULESS_AUTO_RUN_TESTS_ON_BUILD AND TARGET simuless_tests)
            set(SIMULESS_CTEST_ARGS --output-on-failure)
            if(CMAKE_CONFIGURATION_TYPES)
                list(APPEND SIMULESS_CTEST_ARGS -C $<CONFIG>)
            endif()

            add_custom_target(simuless_auto_test ALL
                COMMAND ${CMAKE_CTEST_COMMAND} ${SIMULESS_CTEST_ARGS}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running unit tests with CTest"
            )

            add_dependencies(simuless_auto_test simuless_tests)
        endif()
    endif()
endif()