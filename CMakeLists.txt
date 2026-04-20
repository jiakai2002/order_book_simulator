cmake_minimum_required(VERSION 3.14)
project(order_book)

set(CMAKE_CXX_STANDARD 20)

add_library(order_book src/order_book.cpp)
target_include_directories(order_book PUBLIC include)

add_executable(sim src/main.cpp)
target_link_libraries(sim order_book)

include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googletest)

add_executable(tests tests/test.cpp)
target_link_libraries(tests order_book GTest::gtest_main)

enable_testing()
add_test(NAME order_book_tests COMMAND tests)