cmake_minimum_required(VERSION 3.16)

project(redis-clone)

include(FetchContent)
enable_testing()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

FetchContent_MakeAvailable(googletest)
add_library(resp_lib src/resp_serializer.cpp src/resp_parser.cpp)


add_executable(test_resp tests/test_resp.cpp)
add_executable(milestone5 src/milestone5.cpp)
add_test(NAME RespTests COMMAND test_resp)
target_link_libraries(test_resp PRIVATE resp_lib gtest_main)