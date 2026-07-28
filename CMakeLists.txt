cmake_minimum_required(VERSION 3.20)

project(endweave VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# BDS is built against libc++, and every standard type crossing the plugin boundary must have
# the same layout on both sides. Endstone sets this in its own directory scope, which does not
# reach ours, so set it here too.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif ()

# bedrock-protocol is not public yet. Use the local sibling checkout by default.
# Override with -DBEDROCK_PROTOCOL_SOURCE_DIR=... once it has a public remote.
set(BEDROCK_PROTOCOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bedrock-protocol"
        CACHE PATH "Path to a bedrock-protocol source checkout")
if (NOT EXISTS "${BEDROCK_PROTOCOL_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
            "bedrock-protocol not found at ${BEDROCK_PROTOCOL_SOURCE_DIR}. "
            "Clone https://github.com/EndstoneMC/bedrock-protocol next to this repo "
            "or pass -DBEDROCK_PROTOCOL_SOURCE_DIR=<path>.")
endif ()
add_subdirectory(${BEDROCK_PROTOCOL_SOURCE_DIR} bedrock-protocol EXCLUDE_FROM_ALL)

# The Endstone SDK is header-only: its root CMakeLists returns right after the include/ target
# when consumed as a subproject, so this pulls the headers and endstone_add_plugin(), not the
# server. It also runs Endstone's toolchain checks in our build (Clang or clang-cl, Ninja),
# which is why there is no GCC or MSVC configuration.
include(FetchContent)
set(ENDSTONE_API_VERSION "0.11.6" CACHE STRING "Endstone SDK tag")
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v${ENDSTONE_API_VERSION})
FetchContent_MakeAvailable(endstone)

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
configure_file(src/endweave/version.h.in src/endweave/version.h)

# The translation layer. Split out from the plugin so it can be built and tested without BDS.
add_library(endweave_core STATIC
        src/endweave/protocol/protocol.cpp
        src/endweave/protocol/manager.cpp
        src/endweave/protocol/pipeline.cpp
        src/endweave/connection/connection.cpp
        src/endweave/connection/manager.cpp
        src/endweave/protocols/base/protocol.cpp)
target_include_directories(endweave_core PUBLIC src)
target_link_libraries(endweave_core PUBLIC bedrock::protocol endstone::endstone)

endstone_add_plugin(${PROJECT_NAME}
        src/endweave/plugin.cpp
        src/endweave/listener.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE endweave_core)

# libendstone_runtime.so is LD_PRELOADed, so its libc++ sits in the global lookup scope ahead
# of anything we dlopen. endstone_add_plugin() statically links libc++ at default visibility,
# which would leave two interposing copies in one process. Hide everything but the entry
# point, on our own code and on the static libraries we pull in.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach (target ${PROJECT_NAME} endweave_core bedrock_protocol)
        target_compile_options(${target} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
    endforeach ()
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--exclude-libs,ALL")
endif ()
