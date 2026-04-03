# =========================
# Simuless IO module
# =========================

option(SMLS_IO_USE_LWIP "Build smls_io with lwIP socket backend" OFF)

# Collect all source files under src/
file(GLOB SMLS_IO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# Add only one platform port implementation
if(SMLS_IO_USE_LWIP)
  list(APPEND SMLS_IO_SRC
       ${CMAKE_CURRENT_SOURCE_DIR}/src/port/lwip/smls_socket_port.c)
elseif(WIN32)
  list(APPEND SMLS_IO_SRC
       ${CMAKE_CURRENT_SOURCE_DIR}/src/port/windows/smls_socket_port.c)
else()
  list(APPEND SMLS_IO_SRC
       ${CMAKE_CURRENT_SOURCE_DIR}/src/port/posix/smls_socket_port.c)
endif()

# Create library
add_library(smls_io ${SMLS_IO_SRC})

# Include directories
target_include_directories(
  smls_io
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Platform compile definitions
if(SMLS_IO_USE_LWIP)
  target_compile_definitions(smls_io PRIVATE SMLS_IO_PLATFORM_LWIP=1)
elseif(WIN32)
  target_compile_definitions(smls_io PRIVATE SMLS_IO_PLATFORM_WINDOWS=1)
  target_link_libraries(smls_io PUBLIC ws2_32)
else()
  target_compile_definitions(smls_io PRIVATE SMLS_IO_PLATFORM_POSIX=1)
endif()

# Warning options
target_compile_options(
  smls_io
  PRIVATE $<$<C_COMPILER_ID:GNU>:-Wall -Wextra> $<$<C_COMPILER_ID:Clang>:-Wall
          -Wextra> $<$<C_COMPILER_ID:MSVC>:/W4>)
