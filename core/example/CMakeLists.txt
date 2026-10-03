# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

set(module_files "")
if(nom_build_modules)
    set(module_files example.cppm)
endif()

nom_lib(example
    SOURCES
        counter.cpp
        settings.cpp
    HEADERS
        counter.h
        join.h
        macros.h
        settings.h
        settings_json.h
    MODULES ${module_files}
    PUBLIC_DEPS nlohmann_json::nlohmann_json)

if(nom_install)
    nom_install_package(TARGETS example DEPENDENCIES nlohmann_json)
endif()
