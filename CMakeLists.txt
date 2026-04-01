cmake_minimum_required(VERSION 3.21)
project(endweave VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    endstone
    GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR include
)
FetchContent_MakeAvailable(endstone)

add_library(endweave STATIC
    src/codec/reader.cpp
    src/codec/writer.cpp
    src/codec/wrapper.cpp
    src/codec/types/nbt.cpp
    src/codec/types/actor_data.cpp
    src/codec/types/item.cpp
    src/protocol/packet_ids.cpp
    src/protocol/base.cpp
    src/protocol/sound_rewriter.cpp
)

target_include_directories(endweave PUBLIC include)
target_link_libraries(endweave PRIVATE endstone::endstone)

option(ENDWEAVE_BUILD_TESTS "Build test suite" OFF)
if(ENDWEAVE_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(endweave_tests
        tests/test_codec.cpp
        tests/test_nbt.cpp
        tests/test_protocol.cpp
    )
    target_link_libraries(endweave_tests PRIVATE endweave GTest::gtest_main)
    target_include_directories(endweave_tests PRIVATE include)

    include(GoogleTest)
    gtest_discover_tests(endweave_tests)
endif()
