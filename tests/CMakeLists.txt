add_executable(endweave_tests
        test_attribute_layer_sync.cpp
        test_protocol.cpp
        test_manager.cpp
        test_connection.cpp
        test_pipeline.cpp
        test_base.cpp)
target_link_libraries(endweave_tests PRIVATE endweave_protocol Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(endweave_tests)
