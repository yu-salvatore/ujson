cmake_minimum_required(VERSION 3.14)
project(ujson_tests C)

add_executable(test_conformance test_conformance.c)
target_include_directories(test_conformance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(test_conformance PRIVATE -Wall -Wextra -pedantic)

# Pass test data path to the executable
target_compile_definitions(test_conformance PRIVATE
    TEST_PARSING_DIR="${CMAKE_CURRENT_SOURCE_DIR}/vendor/JSONTestSuite/test_parsing"
)

# Register with CTest
enable_testing()
add_test(NAME conformance COMMAND test_conformance)

# Sanitizer build
option(UJSON_SANITIZERS "Enable ASan + UBSan" OFF)
if(UJSON_SANITIZERS)
    target_compile_options(test_conformance PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_conformance PRIVATE -fsanitize=address,undefined)
endif()
