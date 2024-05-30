cmake_minimum_required(VERSION 3.12)
project(Mini-Container)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set(TARGET mini-container)

file(GLOB SOURCES "src/*.c" "src/*.h")

find_package(OpenSSL REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} OpenSSL::SSL OpenSSL::Crypto)

file(GLOB TEST_SOURCES "tests/*.c")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} OpenSSL::SSL OpenSSL::Crypto)
endforeach()