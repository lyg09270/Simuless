# =========================
# Unit tests
# =========================

file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/*_test.c)

set(SIMULESS_TEST_TARGETS)

function(simuless_target_if_exists out target_name)
  if(TARGET ${target_name})
    set(${out}
        ${target_name}
        PARENT_SCOPE)
  else()
    set(${out}
        ""
        PARENT_SCOPE)
  endif()
endfunction()

simuless_target_if_exists(SIMULESS_LINK_TELEMETRY telemetry)
simuless_target_if_exists(SIMULESS_LINK_OSAL osal)
simuless_target_if_exists(SIMULESS_LINK_TRANSPORT transport)
simuless_target_if_exists(SIMULESS_LINK_SMLS_IO smls_io)
simuless_target_if_exists(SIMULESS_LINK_SMLS_MATH smls_math)
simuless_target_if_exists(SIMULESS_LINK_SMLS_GRAPH smls_graph)
simuless_target_if_exists(SIMULESS_LINK_SMLS_RUNTIME smls_runtime)

foreach(test_file ${TEST_SRC})
  file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${test_file})
  string(REPLACE "/" "_" test_name ${rel_path})
  string(REPLACE ".c" "" test_name ${test_name})

  add_executable(${test_name} ${test_file})

  target_link_libraries(
    ${test_name}
    ${SIMULESS_LINK_TELEMETRY}
    ${SIMULESS_LINK_OSAL}
    ${SIMULESS_LINK_TRANSPORT}
    ${SIMULESS_LINK_SMLS_IO}
    ${SIMULESS_LINK_SMLS_MATH}
    ${SIMULESS_LINK_SMLS_GRAPH}
    ${SIMULESS_LINK_SMLS_RUNTIME})

  add_test(NAME ${test_name} COMMAND ${test_name})
  list(APPEND SIMULESS_TEST_TARGETS ${test_name})
endforeach()

if(SIMULESS_TEST_TARGETS)
  add_custom_target(simuless_tests DEPENDS ${SIMULESS_TEST_TARGETS})
endif()
