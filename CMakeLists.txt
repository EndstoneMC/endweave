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
if (UNIX)
    target_compile_options(bedrock_protocol PUBLIC -stdlib=libc++)
    target_link_options(bedrock_protocol PUBLIC -stdlib=libc++)
endif ()

include(FetchContent)
set(ENDSTONE_API_VERSION "0.11" CACHE STRING "Endstone API version")
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v${ENDSTONE_API_VERSION}
        GIT_SHALLOW TRUE 
)
FetchContent_MakeAvailable(endstone)

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
configure_file(src/endweave/version.h.in src/endweave/version.h)

endstone_add_plugin(${PROJECT_NAME}
src/endweave/connection/manager.cpp
        src/endweave/plugin.cpp
        src/endweave/listener.cpp
        src/endweave/protocols/v1001/transform.cpp
        src/endweave/protocols/v2168/transform.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE bedrock::protocol endstone::endstone)

if (ENDWEAVE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
