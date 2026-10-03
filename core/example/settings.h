// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace nom::example
{
    // A config-like aggregate. Its JSON mapping lives in settings_json.h, outside the module:
    // GCC reports a module exposure warning for nlohmann; CI treats warnings as errors.
    // See cmake_template docs/lab/third_party_in_modules.md for the tested versions.
    struct settings
    {
        std::string name = "world";
        int repeat = 1;
    };

    [[nodiscard]] std::expected<settings, std::string> load_settings(const std::filesystem::path& path);
} // namespace nom::example
