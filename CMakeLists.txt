cmake_minimum_required(VERSION 3.10)
project(tree LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(WITH_PKGTREE "Build with pkgtree runtime support" ON)

set(SOURCES
    main.c
)

if(WITH_PKGTREE)
    list(APPEND SOURCES pkgtree_runtime.c)
endif()

add_executable(tree ${SOURCES})
target_include_directories(tree PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tree PRIVATE m)
