# =========================
# Telemetry module
# =========================

# Collect source files automatically
file(GLOB TELEMETRY_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# Create library
add_library(telemetry ${TELEMETRY_SRC})

# Public include path
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Compile options (module-level safety)
target_compile_options(
  telemetry
  PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall -Wextra> $<$<C_COMPILER_ID:Clang>:-Wall
          -Wextra> $<$<C_COMPILER_ID:MSVC>:/W4>)
