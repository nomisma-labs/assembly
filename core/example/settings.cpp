// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <example/settings.h>
#include <example/settings_json.h>

#include <fstream>

namespace nom::example
{
    std::expected<settings, std::string> load_settings(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file)
            return std::unexpected("cannot open " + path.string());

        const auto json = nlohmann::json::parse(file, nullptr, /*allow_exceptions=*/false);
        if (json.is_discarded())
            return std::unexpected("invalid JSON in " + path.string());
        return json.get<settings>();
    }
} // namespace nom::example
