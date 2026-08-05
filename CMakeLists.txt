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
        src/endweave/protocols/v1001/actor.cpp
        src/endweave/protocols/v1001/block.cpp
        src/endweave/protocols/v1001/chunk.cpp
        src/endweave/protocols/v1001/crafting.cpp
        src/endweave/protocols/v1001/diagnostics.cpp
        src/endweave/protocols/v1001/dimension.cpp
        src/endweave/protocols/v1001/equipment.cpp
        src/endweave/protocols/v1001/game.cpp
        src/endweave/protocols/v1001/input.cpp
        src/endweave/protocols/v1001/inventory.cpp
        src/endweave/protocols/v1001/item_stack.cpp
        src/endweave/protocols/v1001/map.cpp
        src/endweave/protocols/v1001/movement.cpp
        src/endweave/protocols/v1001/player_list.cpp
        src/endweave/protocols/v1001/player_location.cpp
        src/endweave/protocols/v1001/presence.cpp
        src/endweave/protocols/v1001/resource_pack.cpp
        src/endweave/protocols/v1001/scoreboard.cpp
        src/endweave/protocols/v1001/skin.cpp
        src/endweave/protocols/v1001/sound.cpp
        src/endweave/protocols/v1001/structure.cpp
        src/endweave/protocols/v2168/actor.cpp
        src/endweave/protocols/v2168/block.cpp
        src/endweave/protocols/v2168/chunk.cpp
        src/endweave/protocols/v2168/crafting.cpp
        src/endweave/protocols/v2168/diagnostics.cpp
        src/endweave/protocols/v2168/dimension.cpp
        src/endweave/protocols/v2168/equipment.cpp
        src/endweave/protocols/v2168/game.cpp
        src/endweave/protocols/v2168/input.cpp
        src/endweave/protocols/v2168/inventory.cpp
        src/endweave/protocols/v2168/item_stack.cpp
        src/endweave/protocols/v2168/map.cpp
        src/endweave/protocols/v2168/movement.cpp
        src/endweave/protocols/v2168/player_list.cpp
        src/endweave/protocols/v2168/player_location.cpp
        src/endweave/protocols/v2168/presence.cpp
        src/endweave/protocols/v2168/resource_pack.cpp
        src/endweave/protocols/v2168/scoreboard.cpp
        src/endweave/protocols/v2168/skin.cpp
        src/endweave/protocols/v2168/sound.cpp
        src/endweave/protocols/v2168/structure.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE bedrock::protocol endstone::endstone)
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
