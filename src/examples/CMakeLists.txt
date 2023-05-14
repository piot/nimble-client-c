cmake_minimum_required(VERSION 3.17)
project(nimble_client C)

set(CMAKE_C_STANDARD 99)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else()
    message("no debug")
    set(isDebug FALSE)
endif()


if (isDebug)
    message("Debug build detected")
    add_compile_definitions(CONFIGURATION_DEBUG TYRAN_CONFIGURATION_DEBUG)
    set(CONFIGURATION_DEBUG 1)
endif()


set(DEPS ../../deps/)


file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
        "${DEPS}*/*/src/lib/*.c"
        )

message("something ${example_src}")

add_executable(nimble_client
        main.c
        ${deps_src}
    "${DEPS}piot/monotonic-time-c/src/platform/posix/monotonic_time_posix.c"
)

if (WIN32)
target_link_libraries(nimble_client nimble_client_lib)
else()
target_link_libraries(nimble_client PUBLIC nimble_client_lib m)
endif(WIN32)

target_compile_options(nimble_client PRIVATE -Wall -Wextra -Wshadow -Wstrict-aliasing -ansi -pedantic -Wno-declaration-after-statement -Wno-extra-semi-stmt -Wno-undef -Wno-unused-variable -Wno-unused-parameter -Wno-padded -Werror=implicit-function-declaration -Werror=incompatible-pointer-types  -Werror=missing-prototypes -Werror=int-conversion -Werror=return-type -Werror=incompatible-function-pointer-types) # -Werror=incompatible-pointer-types-discards-qualifiers
if (COMPILER_CLANG)
    target_compile_options(nimble_client PRIVATE -Wmost -Weverything -Werror=missing-variable-declarations)
endif()

target_include_directories(nimble_client PRIVATE ${DEPS}piot/clog/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/tiny-libc/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/flood-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/discoid-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/monotonic-time-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/stats-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/nimble-steps-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/blob-stream/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/bit-array/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/nimble-serialize-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/nimble-steps-serialize-c/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/datagram-transport/src/include)
target_include_directories(nimble_client PRIVATE ${DEPS}piot/imprint/src/include)

target_include_directories(nimble_client PRIVATE ${DEPS}piot/udp-client-c/src/include)

target_include_directories(nimble_client PRIVATE ../include)