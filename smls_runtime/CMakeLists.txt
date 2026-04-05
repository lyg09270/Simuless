# =========================
# Simuless Runtime module
# =========================

# Collect all source files
file(GLOB_RECURSE SMLS_RUNTIME_SRC CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# Create static library
add_library(smls_runtime STATIC ${SMLS_RUNTIME_SRC})

# Public headers
target_include_directories(smls_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# C standard
target_compile_features(smls_runtime PUBLIC c_std_99)

# Warning options
target_compile_options(
  smls_runtime
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
