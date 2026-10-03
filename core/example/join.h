// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#pragma once

#include <format>
#include <string>
#include <string_view>

namespace nom::example
{
    // Header-only template: joins formattable values with a separator.
    template <typename... Parts> std::string join([[maybe_unused]] std::string_view separator, const Parts&... parts)
    {
        std::string result;
        std::string_view current_separator;
        ((result += current_separator, result += std::format("{}", parts), current_separator = separator), ...);
        return result;
    }
} // namespace nom::example
