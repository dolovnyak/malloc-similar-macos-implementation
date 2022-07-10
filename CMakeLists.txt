cmake_minimum_required(VERSION 3.21)
project(malloc)

################################################################################
# malloc_lib target
################################################################################
set(MALLOC_LIB "${PROJECT_NAME}_lib")

set(MALLOC_LIB_SOURCES
        "standard_malloc_implementation/malloc.c"
        "standard_malloc_implementation/zone_management.c"
        "standard_malloc_implementation/free.c"
        "standard_malloc_implementation/realloc.c"
        )

add_library(${MALLOC_LIB}
        ${MALLOC_LIB_SOURCES}
        malloc_wrapper.c
        )
target_include_directories(${MALLOC_LIB} PUBLIC standard_malloc_implementation/ ./)

################################################################################
# tests
################################################################################
set(MALLOC_TESTS_LIBRARY ${MALLOC_LIB}_tests)
add_library(${MALLOC_TESTS_LIBRARY} ${MALLOC_LIB_SOURCES})
target_compile_options(${MALLOC_TESTS_LIBRARY} PRIVATE -DGTEST)

set(MALLOC_TESTS ${PROJECT_NAME}_tests)
add_subdirectory(googletest)

add_executable(${MALLOC_TESTS}
        standard_malloc_implementation/tests/malloc_tests.cpp
        standard_malloc_implementation/tests/free_tests.cpp
        standard_malloc_implementation/tests/utilities_tests.cpp
        standard_malloc_implementation/tests/realloc_tests.cpp
        )

target_include_directories(${MALLOC_TESTS} PUBLIC
        googletest/googletest/include
        standard_malloc_implementation)

target_link_libraries(${MALLOC_TESTS}
        gtest
        gtest_main
        ${MALLOC_TESTS_LIBRARY}
        )

# replaced executable file to root dir
set_target_properties(${MALLOC_TESTS}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )