# =========================
# Simuless Graph module
# =========================

# Collect all source files
file(GLOB_RECURSE SMLS_GRAPH_CORE_SRC CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# Create static library
add_library(smls_graph STATIC ${SMLS_GRAPH_CORE_SRC})

# Public headers
target_include_directories(smls_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# C standard
target_compile_features(smls_graph PUBLIC c_std_99)

# Warning options
target_compile_options(
  smls_graph
  PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall
          -Wextra
          -Werror>
          $<$<C_COMPILER_ID:Clang>:-Wall
          -Wextra
          -Werror>
          $<$<C_COMPILER_ID:MSVC>:/W4
          /WX>)

# Platform-specific math library
if(UNIX)
  target_link_libraries(smls_math PUBLIC m)
endif()
