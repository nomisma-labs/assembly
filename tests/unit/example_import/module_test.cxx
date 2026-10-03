// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <example/settings_json.h>

#include <catch2/catch_test_macros.hpp>

import nom.example;

TEST_CASE("the module exports the header API", "[module]")
{
    nom::example::counter clicks;
    clicks.increment();
    CHECK(nom::example::join("-", "clicks", clicks.value()) == "clicks-1");
}

TEST_CASE("settings from the module work with the JSON mapping header", "[module]")
{
    const nom::example::settings original{.name = "module", .repeat = 2};
    const auto restored = nlohmann::json(original).get<nom::example::settings>();
    CHECK(restored.name == original.name);
    CHECK(restored.repeat == original.repeat);
}
