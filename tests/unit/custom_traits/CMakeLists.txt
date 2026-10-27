# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(custom_traits SOURCES custom_traits_test.cxx DEPS nom::assembly)
if(TARGET ${PROJECT_NAME}_custom_traits)
    nom_enable_std_modules(${PROJECT_NAME}_custom_traits)
endif()
