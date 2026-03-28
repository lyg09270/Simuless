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

    # 所有 *_test.c 由上面的 GLOB 自动发现，这里统一创建 target
    add_executable(${test_name} ${test_file})

    target_link_libraries(${test_name}
        telemetry
    )

    if(test_name STREQUAL "transport_tcp_multi_io_test")
        # 仅该测试需要 TCP backend 实现源码
        target_sources(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../transport/src/transport_tcp.c
        )

        target_include_directories(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../transport/inc
        )

        target_compile_definitions(${test_name} PRIVATE
            TRANSPORT_MAX_CHANNELS=8
        )

        if(WIN32)
            target_compile_definitions(${test_name} PRIVATE
                _WINSOCK_DEPRECATED_NO_WARNINGS
            )
            target_link_libraries(${test_name} ws2_32)
        endif()
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})

    list(APPEND SIMULESS_TEST_TARGETS ${test_name})
endforeach()

add_custom_target(simuless_tests DEPENDS ${SIMULESS_TEST_TARGETS})
