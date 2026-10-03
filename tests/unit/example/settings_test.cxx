// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <example/settings.h>
#include <example/settings_json.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

namespace
{
    std::filesystem::path write_file(const std::string& file_name, const std::string& content)
    {
        const auto path = std::filesystem::temp_directory_path() / file_name;
        std::ofstream(path) << content;
        return path;
    }
} // namespace

TEST_CASE("settings load from JSON, with defaults for missing fields", "[settings]")
{
    const auto loaded = nom::example::load_settings(write_file("example_settings_test.json", R"({"name": "lab"})"));
    REQUIRE(loaded);
    CHECK(loaded->name == "lab");
    CHECK(loaded->repeat == 1);
}

TEST_CASE("settings report a missing file and invalid JSON", "[settings]")
{
    CHECK_FALSE(nom::example::load_settings("/nonexistent/settings.json"));
    CHECK_FALSE(nom::example::load_settings(write_file("example_settings_invalid.json", "{ not json")));
}

TEST_CASE("settings round-trip through JSON", "[settings]")
{
    const nom::example::settings original{.name = "round", .repeat = 3};
    const auto restored = nlohmann::json(original).get<nom::example::settings>();
    CHECK(restored.name == original.name);
    CHECK(restored.repeat == original.repeat);
}
