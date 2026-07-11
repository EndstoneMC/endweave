cmake_minimum_required(VERSION 3.20)

project(endweave VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENDWEAVE_BUILD_PLUGIN "Build the Endstone plugin (fetches the Endstone SDK)" ON)
option(ENDWEAVE_BUILD_TESTS "Build the translation test suite" ON)

# bedrock-protocol is not public yet. Use the local sibling checkout by default;
# override with -DBEDROCK_PROTOCOL_SOURCE_DIR=... once it has a public remote.
set(BEDROCK_PROTOCOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bedrock-protocol"
        CACHE PATH "Path to a bedrock-protocol source checkout")
if (NOT EXISTS "${BEDROCK_PROTOCOL_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
            "bedrock-protocol not found at ${BEDROCK_PROTOCOL_SOURCE_DIR}. "
            "Clone https://github.com/EndstoneMC/bedrock-protocol next to this repo "
            "or pass -DBEDROCK_PROTOCOL_SOURCE_DIR=<path>.")
endif ()
add_subdirectory(${BEDROCK_PROTOCOL_SOURCE_DIR} bedrock-protocol EXCLUDE_FROM_ALL)

# Translation layer: a pure codec consumer with no Endstone dependency, so it
# builds (and the tests run) even when the plugin is gated off.
add_library(endweave_protocol STATIC
        src/protocol/attribute_layer_sync.cpp
        src/protocol/protocol.cpp
        src/protocol/v975_to_v1001.cpp
        src/protocol/v1001_to_v975.cpp)
target_include_directories(endweave_protocol PUBLIC include)
target_link_libraries(endweave_protocol PUBLIC bedrock::protocol)
target_compile_features(endweave_protocol PUBLIC cxx_std_23)

if (ENDWEAVE_BUILD_PLUGIN)
    include(FetchContent)
    set(ENDSTONE_API_VERSION "0.11" CACHE STRING "Endstone API version")
    FetchContent_Declare(
            endstone
            GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
            GIT_TAG v${ENDSTONE_API_VERSION})
    FetchContent_MakeAvailable(endstone)

    string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
    configure_file(include/endweave/version.h.in include/endweave/version.h)

    endstone_add_plugin(${PROJECT_NAME} src/plugin.cpp)
    target_include_directories(${PROJECT_NAME} PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/include)
endif ()

if (ENDWEAVE_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.5.4)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    add_subdirectory(tests)
endif ()
