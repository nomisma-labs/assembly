// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <example/join.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("join formats and separates its arguments", "[join]")
{
    CHECK(nom::example::join(", ") == "");
    CHECK(nom::example::join(", ", "one") == "one");
    CHECK(nom::example::join(", ", "one", 2, 3.5) == "one, 2, 3.5");
}
