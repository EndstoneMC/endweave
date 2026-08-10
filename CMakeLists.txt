cmake_minimum_required(VERSION 3.20)

project(endweave VERSION 0.5.0 LANGUAGES CXX)

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
# endstone_add_plugin hides our own symbols, but bedrock_protocol is a target of its own and
# would otherwise link its serialisers into the plugin at default visibility.
set_target_properties(bedrock_protocol PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
if (UNIX)
    target_compile_options(bedrock_protocol PUBLIC -stdlib=libc++)
    target_link_options(bedrock_protocol PUBLIC -stdlib=libc++)
endif ()

include(FetchContent)
find_package(tomlplusplus QUIET)
if (NOT tomlplusplus_FOUND)
    FetchContent_Declare(
            tomlplusplus
            GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
            GIT_TAG v3.3.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif ()

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
        src/endweave/config.cpp
        src/endweave/connection/manager.cpp
        src/endweave/plugin.cpp
        src/endweave/listener.cpp)
add_subdirectory(src/endweave/protocols/v1001)
add_subdirectory(src/endweave/protocols/v2168)
add_subdirectory(src/endweave/protocols/v2181)
target_include_directories(${PROJECT_NAME} PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE bedrock::protocol endstone::endstone tomlplusplus::tomlplusplus)
# Report a malformed config through the return value rather than over the plugin boundary.
target_compile_definitions(${PROJECT_NAME} PRIVATE TOML_EXCEPTIONS=0)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" OUTPUT_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
if (UNIX)
    # bedrock_protocol and the static libc++ endstone_add_plugin links arrive as archives, whose
    # symbols stay exported even with hidden visibility on every target.
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--exclude-libs,ALL)
endif ()

# Install into a server: the plugin endstone loads.
install(TARGETS ${PROJECT_NAME} COMPONENT plugin
        LIBRARY DESTINATION plugins RUNTIME DESTINATION plugins)

if (ENDWEAVE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
