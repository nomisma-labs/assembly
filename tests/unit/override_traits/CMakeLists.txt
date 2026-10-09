# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(override_traits SOURCES override_traits_test.cxx DEPS nom::assembly)
set_tests_properties(assembly_override_traits PROPERTIES LABELS unit)
