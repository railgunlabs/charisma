cmake_minimum_required(VERSION 3.21)
project(charisma VERSION 1.0.3 LANGUAGES C)

include(CheckCSourceCompiles)
include(CMakePackageConfigHelpers)

set(CHARISMA_TARGET_ENDIAN "auto" CACHE STRING "Endianness of the target hardware")
set_property(CACHE CHARISMA_TARGET_ENDIAN PROPERTY STRINGS auto little big)

# Check the endianness of the target hardware.
# When 'auto' is used assume the same endian as the hardware the code is compiled on.
if (CHARISMA_TARGET_ENDIAN STREQUAL "auto")
    if (CMAKE_C_BYTE_ORDER EQUAL BIG_ENDIAN)
        set(HAVE_BIG_ENDIAN 1)
    else ()
        set(HAVE_BIG_ENDIAN 0)
    endif ()
elseif (CHARISMA_TARGET_ENDIAN STREQUAL "big")
    set(HAVE_BIG_ENDIAN 1)
elseif (CHARISMA_TARGET_ENDIAN STREQUAL "little")
    set(HAVE_BIG_ENDIAN 0)
endif ()

# Report the endianness.
if (HAVE_BIG_ENDIAN)
    message(STATUS "Target system is Big Endian")
else ()
    message(STATUS "Target system is Little Endian")
endif ()

# Check for FreeBSD bswap functions.
if (NOT HAVE_BSWAP)
    check_c_source_compiles(
        "
        #include <sys/endian.h>
        #include <stdint.h>
        int main(int argc, char *argv[]) {
            uint16_t x;
            uint32_t y;
            x = bswap16(&x);
            y = bswap32(&y);
            return 0;
        }
        "
        HAVE_FREEBSD_BSWAP
    )
    if (HAVE_FREEBSD_BSWAP)
        set(HAVE_BSWAP 1)
    endif ()
endif ()

# Check for Linux bswap functions.
if (NOT HAVE_BSWAP)
    check_c_source_compiles(
        "
        #include <byteswap.h>
        #include <stdint.h>
        int main(int argc, char *argv[]) {
            uint16_t x;
            uint32_t y;
            x = bswap_16(&x);
            y = bswap_32(&y);
            return 0;
        }
        "
        HAVE_LINUX_BSWAP
    )
    if (HAVE_LINUX_BSWAP)
        set(HAVE_BSWAP 1)
    endif ()
endif ()

# Check for compiler intrinsic functions.
if (NOT HAVE_BSWAP)
    check_c_source_compiles(
        "
        #include <stdint.h>
        int main(int argc, char *argv[]) {
            uint16_t x;
            uint32_t y;
            x = __builtin_bswap16(&x);
            y = __builtin_bswap32(&y);
            return 0;
        }
        "
        HAVE_COMPILER_BSWAP
    )
    if (HAVE_COMPILER_BSWAP)
        set(HAVE_BSWAP 1)
    endif ()
endif ()

# Check for MSVC CRT functions (Windows)
if (NOT HAVE_BSWAP)
    check_c_source_compiles(
        "
        #include <stdint.h>
        #include <stdlib.h>
        int main(int argc, char *argv[]) {
            uint16_t x;
            uint32_t y;
            if (sizeof(uint16_t) != sizeof(unsigned short) ||
                sizeof(uint32_t) != sizeof(unsigned long)) {
                return 1;
            }
            x = _byteswap_ushort(&x);
            y = _byteswap_ulong(&y);
            return 0;
        }
        "
        HAVE_MSVC_BYTESWAP
    )
    if (HAVE_MSVC_BYTESWAP)
        set(HAVE_BSWAP 1)
    endif ()
endif ()

# Generate version information.
write_basic_package_version_file(${CMAKE_BINARY_DIR}/CharismaConfigVersion.cmake COMPATIBILITY SameMajorVersion)

# Generate configuration for CMake's find package tools.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CharismaConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/CharismaConfig.cmake" @ONLY)

# Generate the config.h header file and include its path in all targets.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_compile_definitions(HAVE_CONFIG_H=1)

add_library(charisma_static STATIC charisma.c charisma.h)
set_target_properties(charisma_static PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/charisma.h")
target_compile_definitions(charisma_static PUBLIC -DCHARISMA_STATIC)

add_library(charisma SHARED charisma.c charisma.h)
set_target_properties(charisma PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/charisma.h")
set_target_properties(charisma PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(charisma PRIVATE -DDLL_EXPORT)

if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR
    CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_compile_options(charisma PRIVATE -pedantic)
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(charisma PRIVATE -Wall -Wextra -Wconversion -Wno-missing-field-initializers
        -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion)
endif ()

install(TARGETS charisma_static ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(TARGETS charisma ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CharismaConfig.cmake DESTINATION cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CharismaConfigVersion.cmake DESTINATION cmake)
