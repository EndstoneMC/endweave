cmake_minimum_required(VERSION 3.20)

project(endweave VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TODO: bedrock-protocol is not public yet. Use the local sibling checkout by default.
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

include(FetchContent)
set(ENDSTONE_API_VERSION "0.11" CACHE STRING "Endstone API version")
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v${ENDSTONE_API_VERSION})
FetchContent_MakeAvailable(endstone)

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
configure_file(src/endweave/version.h.in src/endweave/version.h)

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

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach (target ${PROJECT_NAME} endweave_core bedrock_protocol)
        target_compile_options(${target} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
    endforeach ()
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--exclude-libs,ALL")
endif ()
