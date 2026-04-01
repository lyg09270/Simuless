# =========================
# OSAL (Operating System Abstraction Layer)
# =========================

# =========================
# Select platform sources
# =========================

if(WIN32)
  file(GLOB_RECURSE OSAL_SRC CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/windows/*.c)
elseif(UNIX)
  file(GLOB_RECURSE OSAL_SRC CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/posix/*.c)
endif()

# =========================
# Create library
# =========================

add_library(osal STATIC ${OSAL_SRC})

# =========================
# Include paths
# =========================

target_include_directories(osal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# =========================
# Compile options
# =========================

target_compile_options(osal PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
                                    $<$<C_COMPILER_ID:MSVC>:/W4>)

# =========================
# Platform defines
# =========================

if(WIN32)
  target_compile_definitions(osal PRIVATE OSAL_PLATFORM_WINDOWS=1)
elseif(UNIX)
  target_compile_definitions(osal PRIVATE OSAL_PLATFORM_POSIX=1)
endif()
