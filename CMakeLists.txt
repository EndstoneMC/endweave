cmake_minimum_required(VERSION 3.28)

project(endweave VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BEDROCK_PROTOCOL_VERSION "0.1.0" CACHE STRING "bedrock-protocol release to build against")
set(BEDROCK_PROTOCOL_SOURCE_DIR "" CACHE PATH "Local bedrock-protocol checkout, for schema work")

include(FetchContent)
if (BEDROCK_PROTOCOL_SOURCE_DIR)
    add_subdirectory(${BEDROCK_PROTOCOL_SOURCE_DIR} bedrock-protocol EXCLUDE_FROM_ALL)
else ()
    FetchContent_Declare(bedrock_protocol
            URL https://github.com/EndstoneMC/bedrock-protocol/releases/download/v${BEDROCK_PROTOCOL_VERSION}/bedrock-protocol-${BEDROCK_PROTOCOL_VERSION}-src.zip
            URL_HASH SHA256=fcac3113e2dcf7c3f81d2510b492258e762c92ddb4b392c214490eebfe4492a3
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(bedrock_protocol)
endif ()

set_target_properties(bedrock_protocol PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
if (NOT SKBUILD)
    message(STATUS "endweave: building outside scikit-build-core; the wheel is built with 'pip wheel .'")
endif ()

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module
        OPTIONAL_COMPONENTS Development.SABIModule)

execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR
        RESULT_VARIABLE NB_RESULT)
if (NOT NB_RESULT EQUAL 0)
    message(FATAL_ERROR "nanobind not found for ${Python_EXECUTABLE}. Install it with: pip install nanobind")
endif ()
list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
find_package(nanobind CONFIG REQUIRED)

if (SKBUILD_SABI_VERSION AND NOT TARGET Python::SABIModule)
    message(FATAL_ERROR "endweave: a stable-ABI wheel was asked for and Development.SABIModule was not found, "
            "so nanobind would fall back to a version-specific build and tag the wheel wrongly.")
endif ()

nanobind_add_module(_pipeline STABLE_ABI NB_STATIC src/endweave/_pipeline.cpp)
target_include_directories(_pipeline PRIVATE src)
target_link_libraries(_pipeline PRIVATE bedrock::protocol)
set_target_properties(_pipeline PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_subdirectory(src/endweave/protocols)

if (UNIX)
    target_link_options(_pipeline PRIVATE -Wl,--exclude-libs,ALL)
    target_link_libraries(_pipeline PRIVATE -static-libstdc++ -static-libgcc)
endif ()

nanobind_add_stub(_pipeline_stub
        MODULE _pipeline
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/endweave/_pipeline.pyi"
        PYTHON_PATH $<TARGET_FILE_DIR:_pipeline>
        DEPENDS _pipeline
        MARKER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/endweave/py.typed"
)

install(TARGETS _pipeline COMPONENT python DESTINATION endweave)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/endweave/_version.py")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/endweave/_version.py"
            COMPONENT python DESTINATION endweave)
endif ()
