# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_app(hello SOURCES main.cpp DEPS nom::example)

if(nom_build_modules)
    nom_app(hello_import SOURCES main_import.cpp DEPS nom::example)
endif()
