cmake_minimum_required(VERSION 3.21)
project(charisma VERSION 1.1.0 LANGUAGES C)

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

# Check for POSIX functions which are required to build the command-line interface.
check_c_source_compiles(
    "
    #include <unistd.h>
    int main(int argc, char *argv[]) {
        char buffer[8];
        ssize_t r = read(STDIN_FILENO, buffer, sizeof(buffer));
        return 0;
    }
    "
    HAVE_POSIX_ENVIRONMENT
)

# Generate version information.
write_basic_package_version_file(${CMAKE_BINARY_DIR}/CharismaConfigVersion.cmake COMPATIBILITY SameMajorVersion)

# Generate configuration for CMake's find package tools.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CharismaConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/CharismaConfig.cmake" @ONLY)

# Generate the config.h header file and include its path in all targets.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_compile_definitions(HAVE_CONFIG_H=1)

add_library(charisma STATIC charisma.c charisma.h)
set_target_properties(charisma PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/charisma.h")

if (HAVE_POSIX_ENVIRONMENT)
    add_executable(charisma_cli extras/charisma-cli.c charisma.c charisma.h)
    target_include_directories(charisma_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(charisma_cli PROPERTIES OUTPUT_NAME "charisma")
else ()
    message(WARNING "Skipping building the command-line interface because it requires a POSIX-compliant environment.")
endif ()

add_subdirectory(man)

if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR
    CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_compile_options(charisma PRIVATE -pedantic)
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(charisma PRIVATE -Wall -Wextra -Wconversion -Wno-missing-field-initializers
        -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion)
endif ()

install(TARGETS charisma ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(TARGETS charisma_cli RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CharismaConfig.cmake DESTINATION cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CharismaConfigVersion.cmake DESTINATION cmake)
