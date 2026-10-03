// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#pragma once

#include <example/settings.h>

#include <nlohmann/json.hpp>

namespace nom::example
{
    // A public header that includes nlohmann makes nlohmann_json a public dependency: the case
    // the config repo has with DEFINE_CONFIG.
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(settings, name, repeat)
} // namespace nom::example
