find_package(GTest CONFIG REQUIRED)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(tar_tests ${TEST_SOURCES})

target_include_directories(tar_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tar_tests PRIVATE
    GTest::gtest_main
    installer_lib
)

include(GoogleTest)
gtest_discover_tests(tar_tests)
