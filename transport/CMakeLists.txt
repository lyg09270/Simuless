# =========================
# Transport module
# =========================

# Collect source files automatically
file(GLOB TRANSPORT_SRC
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Collect header files (optional, for IDE visibility)
file(GLOB TRANSPORT_INC
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h
)

# Create library
add_library(transport
    ${TRANSPORT_SRC}
    ${TRANSPORT_INC}
)

# Public include path
target_include_directories(transport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

if (WIN32)
    target_link_libraries(transport PUBLIC ws2_32)
endif()

# Compile options (module-level safety)
target_compile_options(transport PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<C_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<C_COMPILER_ID:MSVC>:/W4>
)