# =========================
# Unit tests
# =========================

file(GLOB TEST_SRC
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.c
)

set(SIMULESS_TEST_TARGETS)

foreach(test_file ${TEST_SRC})
    get_filename_component(test_name ${test_file} NAME_WE)

    add_executable(${test_name} ${test_file})

    target_link_libraries(${test_name}
        telemetry
    )

    add_test(NAME ${test_name} COMMAND ${test_name})

    list(APPEND SIMULESS_TEST_TARGETS ${test_name})
endforeach()

add_custom_target(simuless_tests DEPENDS ${SIMULESS_TEST_TARGETS})
