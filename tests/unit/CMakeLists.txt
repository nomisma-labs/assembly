# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

# One subdirectory per Catch2 unit test suite (docs/howto.md#a-unit-test-suite).

# template-only:begin
add_subdirectory(example)
if(nom_build_modules)
    add_subdirectory(example_import)
endif()
# template-only:end
