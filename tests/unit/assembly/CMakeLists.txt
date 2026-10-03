# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(assembly SOURCES assembly_test.cxx DEPS nom::assembly)
set_tests_properties(assembly_assembly PROPERTIES LABELS unit)
