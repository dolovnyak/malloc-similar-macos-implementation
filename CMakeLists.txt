cmake_minimum_required(VERSION 3.21)
project(ft_malloc DESCRIPTION "MacOS malloc implementation")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3")

################################################################################
# malloc_lib target
################################################################################

if (NOT DEFINED $ENV{HOSTTYPE})
    execute_process (COMMAND bash -c "uname -m" OUTPUT_VARIABLE MACHINE_NAME)
    string(STRIP ${MACHINE_NAME} MACHINE_NAME)
    execute_process (COMMAND bash -c "uname -s" OUTPUT_VARIABLE KERNEL_NAME)
    string(STRIP ${KERNEL_NAME} KERNEL_NAME)
    set(ENV{HOSTTYPE} ${MACHINE_NAME}_${KERNEL_NAME})
endif()

set(MALLOC_LIB ${PROJECT_NAME}_$ENV{HOSTTYPE})

set(MALLOC_LIB_SOURCES
        "macos_similar_malloc_implementation/malloc.c"
        "macos_similar_malloc_implementation/zone_management.c"
        "macos_similar_malloc_implementation/free.c"
        "macos_similar_malloc_implementation/realloc.c"
        "macos_similar_malloc_implementation/mem_dump.c"
        )

add_library(${MALLOC_LIB} SHARED
        ${MALLOC_LIB_SOURCES}
        malloc_wrapper.c
        )
target_include_directories(${MALLOC_LIB} PRIVATE macos_similar_malloc_implementation/ ./)

set_target_properties(${MALLOC_LIB} PROPERTIES PUBLIC_HEADER malloc.h)
set_target_properties(${MALLOC_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${MALLOC_LIB} PROPERTIES SUFFIX ".so")

################################################################################
# example
################################################################################
add_executable(example main_example.c)
target_include_directories(example PUBLIC ./)
target_link_libraries(example ${MALLOC_LIB})

set_target_properties(example
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )

################################################################################
# tests
################################################################################
set(MALLOC_TESTS_LIBRARY ${MALLOC_LIB}_tests)
add_library(${MALLOC_TESTS_LIBRARY} ${MALLOC_LIB_SOURCES})
target_compile_options(${MALLOC_TESTS_LIBRARY} PRIVATE -DGTEST)

set(MALLOC_TESTS ${PROJECT_NAME}_tests)
add_subdirectory(googletest)

add_executable(${MALLOC_TESTS}
        macos_similar_malloc_implementation/tests/malloc_tests.cpp
        macos_similar_malloc_implementation/tests/free_tests.cpp
        macos_similar_malloc_implementation/tests/utilities_tests.cpp
        macos_similar_malloc_implementation/tests/realloc_tests.cpp
        )

target_include_directories(${MALLOC_TESTS} PUBLIC
        googletest/googletest/include
        macos_similar_malloc_implementation)

target_link_libraries(${MALLOC_TESTS}
        gtest
        gtest_main
        ${MALLOC_TESTS_LIBRARY}
        )

set_target_properties(${MALLOC_TESTS}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )