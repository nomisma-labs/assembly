# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

cmake_minimum_required(VERSION 3.30)
project(assembly_library_usage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(LIBRARY_USAGE_MODE STREQUAL "package")
    find_package(assembly CONFIG REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(assembly SOURCE_DIR ${LIBRARY_USAGE_SOURCE_DIR})
    FetchContent_MakeAvailable(assembly)
endif()

enable_testing()
add_executable(use_library use_library_test.cxx)
target_link_libraries(use_library PRIVATE nom::assembly)
add_test(NAME use_library COMMAND use_library)
