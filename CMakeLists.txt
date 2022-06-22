cmake_minimum_required(VERSION 3.21)
project(malloc)

################################################################################
# malloc_lib target
################################################################################
set(MALLOC_LIB "${PROJECT_NAME}_lib")
add_library(${MALLOC_LIB}
        standard_malloc_implementation/malloc.c
        standard_malloc_implementation/zone_management.c
        )
target_include_directories(${MALLOC_LIB} PUBLIC standard_malloc_implementation/)


################################################################################
# tests
################################################################################
set(MALLOC_TESTS ${PROJECT_NAME}_tests)
add_subdirectory(googletest)

add_executable(${MALLOC_TESTS}
        standard_malloc_implementation/tests/malloc_tests.cpp)

target_include_directories(${MALLOC_TESTS} PUBLIC
        googletest/googletest/include
        standard_malloc_implementation)

target_link_libraries(${MALLOC_TESTS}
        gtest
        gtest_main
        ${MALLOC_LIB}
        )

# replaced executable file to root dir
set_target_properties(${MALLOC_TESTS}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
