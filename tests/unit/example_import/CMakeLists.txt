# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

# The same API through `import nom.example;` (module mode).
nom_c2test(import_tests SOURCES module_test.cxx DEPS nom::example)
set_tests_properties(${PROJECT_NAME}_import_tests PROPERTIES LABELS unit)
