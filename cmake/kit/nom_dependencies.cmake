# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

include(FetchContent)

# nom_require_submodule(<path>)
#
# Fails early, with the fix, when a submodule was not checked out: a plain `git clone` leaves
# external/ empty. Only checked for a standalone build; in a superbuild the outer project
# supplies the dependency.
function(nom_require_submodule path)
    if(NOT PROJECT_IS_TOP_LEVEL)
        return()
    endif()
    file(GLOB entries ${PROJECT_SOURCE_DIR}/${path}/*)
    if(NOT entries)
        message(FATAL_ERROR "${path} is empty. Run: git submodule update --init --recursive")
    endif()
endfunction()

# nom_find_catch2()
#
# The system Catch2 3 when installed, otherwise a pinned download.
macro(nom_find_catch2)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.16.0
        GIT_SHALLOW TRUE
        FIND_PACKAGE_ARGS 3 CONFIG)
    FetchContent_MakeAvailable(Catch2)
endmacro()
