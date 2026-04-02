include(Catch)

file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(cc-make-tests ${TEST_SOURCES})
target_link_libraries(cc-make-tests PRIVATE
    cc-make-lib
    Catch2::Catch2WithMain
)
catch_discover_tests(cc-make-tests)
