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

# all the reusable logic
add_library(resp_lib 
    src/resp_serializer.cpp 
    src/resp_parser.cpp 
    src/commands/general_commands.cpp
    src/commands/string_commands.cpp
    src/commands/list_commands.cpp
    src/commands/hash_commands.cpp
    src/commands/set_commands.cpp
    src/datastore.cpp
    src/utils.cpp
    src/command_handler.cpp)


# Main Server
add_executable(server src/server.cpp)
target_link_libraries(server PRIVATE resp_lib)

# Tests
add_executable(test_resp tests/test_resp.cpp)
target_link_libraries(test_resp PRIVATE resp_lib gtest_main)
add_test(NAME RespTests COMMAND test_resp)