# =========================
# Simuless Math module
# =========================

# Collect all source files under src/
file(GLOB_RECURSE SMLS_MATH_CORE_SRC CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/math_core/*.c)

# Create library
add_library(smls_math STATIC ${SMLS_MATH_CORE_SRC})

# Include directories
target_include_directories(smls_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Warning options
target_compile_options(
  smls_math
  PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall -Wextra> $<$<C_COMPILER_ID:Clang>:-Wall
          -Wextra> $<$<C_COMPILER_ID:MSVC>:/W4>)
