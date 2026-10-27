# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(relay SOURCES relay_test.cxx DEPS nom::assembly)
if(TARGET ${PROJECT_NAME}_relay)
    nom_enable_std_modules(${PROJECT_NAME}_relay)
endif()
