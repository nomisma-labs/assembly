# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(complex_relay SOURCES complex_relay_test.cxx DEPS nom::assembly)
set_tests_properties(assembly_complex_relay PROPERTIES LABELS unit)
