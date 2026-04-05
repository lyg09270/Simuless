# =========================
# Simuless Math module
# =========================

# Collect all source files under src/
file(GLOB SMLS_MATH_CORE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/math_core/*.c)
file(GLOB SMLS_MATH_ESTIMATION_SRC ${CMAKE_CURRENT_SOURCE_DIR}/estimation/*.c)
file(GLOB SMLS_MATH_CONTROL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/control/*.c)
file(GLOB SMLS_MATH_SIGNAL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/signal/*.c)
file(GLOB SMLS_MATH_SYSTEM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/system/*.c)

# Create library
add_library(
  smls_math
  ${SMLS_MATH_CORE_SRC} ${SMLS_MATH_ESTIMATION_SRC} ${SMLS_MATH_CONTROL_SRC}
  ${SMLS_MATH_SIGNAL_SRC} ${SMLS_MATH_SYSTEM_SRC})

# Include directories
target_include_directories(smls_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Warning options
target_compile_options(
  smls_math
  PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall -Wextra> $<$<C_COMPILER_ID:Clang>:-Wall
          -Wextra> $<$<C_COMPILER_ID:MSVC>:/W4>)
