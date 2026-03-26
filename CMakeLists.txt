cmake_minimum_required(VERSION 3.10)
project(tree VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(WITH_PKGTREE "Build with pkgtree runtime support" ON)

set(SOURCES main.c)
if(WITH_PKGTREE)
    list(APPEND SOURCES pkgtree_runtime.c)
endif()

add_executable(tree ${SOURCES})
target_include_directories(tree PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tree PRIVATE m)

## Install
install(TARGETS tree RUNTIME DESTINATION bin)
install(FILES pkgtree_runtime.h README.md DESTINATION share/doc/tree)

## Packaging via CPack
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "tree")
set(CPACK_PACKAGE_VENDOR "Bitraxis")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tree language interpreter and pkgtree runtime")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ")
include(CPack)
