# =========================
# Telemetry module
# =========================

# Collect source files automatically
file(GLOB TELEMETRY_SRC
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Collect header files (optional, for IDE visibility)
file(GLOB TELEMETRY_INC
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
)

# Create library
add_library(telemetry
    ${TELEMETRY_SRC}
    ${TELEMETRY_INC}
)

# Public include path
target_include_directories(telemetry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Compile options (module-level safety)
target_compile_options(telemetry PRIVATE
    -Wall
    -Wextra
)