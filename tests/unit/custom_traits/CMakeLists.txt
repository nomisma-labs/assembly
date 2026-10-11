# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(custom_traits SOURCES custom_traits_test.cxx DEPS nom::assembly)
set_tests_properties(assembly_custom_traits PROPERTIES LABELS unit)
